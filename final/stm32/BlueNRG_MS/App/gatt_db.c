/**
  ******************************************************************************
  * @file    App/gatt_db.c
  * @author  SRA Application Team
  * @brief   Functions to build GATT DB and handle GATT events
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bluenrg_def.h"
#include "gatt_db.h"
#include "bluenrg_conf.h"
#include "bluenrg_gatt_aci.h"

#include <stdint.h>
#include <stdio.h>

// Define the thresholds
#define POSITIVE_THRESHOLD 950
#define NEGATIVE_THRESHOLD -550

// Define the window size for the weighted moving average
#define WINDOW_SIZE 2
#define SUB_WINDOW_SIZE 2

// Define a buffer size for storing values
#define BUFFER_SIZE 64


/** @brief Macro that stores Value into a buffer in Little Endian Format (2 bytes)*/
#define HOST_TO_LE_16(buf, val)    ( ((buf)[0] =  (uint8_t) (val)    ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8) ) )

/** @brief Macro that stores Value into a buffer in Little Endian Format (4 bytes) */
#define HOST_TO_LE_32(buf, val)    ( ((buf)[0] =  (uint8_t) (val)     ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8)  ) , \
                                   ((buf)[2] =  (uint8_t) (val>>16) ) , \
                                   ((buf)[3] =  (uint8_t) (val>>24) ) )

#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
do {\
    uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; uuid_struct[2] = uuid_2; uuid_struct[3] = uuid_3; \
        uuid_struct[4] = uuid_4; uuid_struct[5] = uuid_5; uuid_struct[6] = uuid_6; uuid_struct[7] = uuid_7; \
            uuid_struct[8] = uuid_8; uuid_struct[9] = uuid_9; uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
                uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
}while(0)

/* Hardware Characteristics Service */
#define COPY_HW_SENS_W2ST_SERVICE_UUID(uuid_struct)    COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x01,0x11,0xe1,0x9a,0xb4,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_ENVIRONMENTAL_W2ST_CHAR_UUID(uuid_struct) COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_ACC_GYRO_MAG_W2ST_CHAR_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x00,0xE0,0x00,0x00,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
/* Software Characteristics Service */
#define COPY_SW_SENS_W2ST_SERVICE_UUID(uuid_struct)    COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x02,0x11,0xe1,0x9a,0xb4,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_QUATERNIONS_W2ST_CHAR_UUID(uuid_struct)   COPY_UUID_128(uuid_struct,0x00,0x00,0x01,0x00,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)

uint16_t HWServW2STHandle, EnvironmentalCharHandle, AccGyroMagCharHandle, AccDataCharHandle;
uint16_t SWServW2STHandle, QuaternionsCharHandle;

/* UUIDS */
Service_UUID_t service_uuid;
Char_UUID_t char_uuid;

extern AxesRaw_t x_axes;
extern AxesRaw_t g_axes;
extern AxesRaw_t m_axes;

extern uint16_t connection_handle;
extern uint32_t start_time;
extern uint8_t sample_rate;
//uint8_t sample_rate = 1;
// sample_rate = 1;

// Buffers for storing x, y, z values
int16_t x_values[BUFFER_SIZE];
int16_t y_values[BUFFER_SIZE];
int16_t z_values[BUFFER_SIZE];
int buffer_index = 0;

// Variables to track threshold exceedance
int threshold_exceeded = 0;
int last_exceeded_threshold = 0;

// Function to compute the weighted moving average
float weighted_moving_average(int16_t* values, int length) {
    int sum_weights = 0;
    int weighted_sum = 0;

    // Ensure the window size is at least 2, else return the latest value
    if (length < 2) {
        return values[length - 1];  // Return the most recent value if not enough data
    } else {
        // For the last two values, apply weights 2 for the most recent, 1 for the previous
        sum_weights = 3;  // 2 + 1
        weighted_sum = values[length - 1] * 2 + values[length - 2] * 1;
    }
    return (float)weighted_sum / sum_weights;
}

// Function to check the thresholds and send the result over Bluetooth
void check_thresholds_and_send(float value) {
    uint8_t buff = 0;  // Default to idle (0)

    if (!threshold_exceeded) {
        if (value > POSITIVE_THRESHOLD) {
            threshold_exceeded = 1;
            last_exceeded_threshold = 1;
            buff = 1;  // Notify 'up' (positive threshold exceeded)
        } else if (value < NEGATIVE_THRESHOLD) {
            threshold_exceeded = 1;
            last_exceeded_threshold = -1;
            buff = 2;  // Notify 'down' (negative threshold exceeded)
        }
    } else {
        int start = buffer_index - SUB_WINDOW_SIZE;
        if (start < 0) start += BUFFER_SIZE;
        int count = 0;

        if (last_exceeded_threshold == 1) {
            // Check if the values are below the positive threshold
            for (int i = 0; i < SUB_WINDOW_SIZE; ++i) {
                int idx = (start + i) % BUFFER_SIZE;
                if (y_values[idx] <= POSITIVE_THRESHOLD) {
                    count++;
                }
            }
            if (count == SUB_WINDOW_SIZE) {
                threshold_exceeded = 0;  // Reset if all recent values are below the threshold
            }
        } else if (last_exceeded_threshold == -1) {
            // Check if the values are above the negative threshold
            for (int i = 0; i < SUB_WINDOW_SIZE; ++i) {
                int idx = (start + i) % BUFFER_SIZE;
                if (y_values[idx] >= NEGATIVE_THRESHOLD) {
                    count++;
                }
            }
            if (count == SUB_WINDOW_SIZE) {
                threshold_exceeded = 0;  // Reset if all recent values are above the threshold
            }
        }
    }

    // Send the determined value (0 for idle, 1 for up, -1 for down)
    aci_gatt_update_char_value(HWServW2STHandle, AccDataCharHandle, 0, 1, &buff);
}



/**
 * @brief  Add the 'HW' service (and the Environmental and AccGyr characteristics).
 * @param  None
 * @retval tBleStatus Status
 */
tBleStatus Add_HWServW2ST_Service(void)
{
  tBleStatus ret;
  uint8_t uuid[16];

  /* Add_HWServW2ST_Service */
  COPY_HW_SENS_W2ST_SERVICE_UUID(uuid);
  BLUENRG_memcpy(&service_uuid.Service_UUID_128, uuid, 16);
  ret = aci_gatt_add_serv(UUID_TYPE_128, service_uuid.Service_UUID_128, PRIMARY_SERVICE,
                          1+3*5, &HWServW2STHandle);
  if (ret != BLE_STATUS_SUCCESS)
    return BLE_STATUS_ERROR;

  /* Fill the Environmental BLE Characteristc */
  COPY_ENVIRONMENTAL_W2ST_CHAR_UUID(uuid);
  BLUENRG_memcpy(&char_uuid.Char_UUID_128, uuid, 16);
  ret =  aci_gatt_add_char(HWServW2STHandle, UUID_TYPE_128, char_uuid.Char_UUID_128,
                           6,
                           CHAR_PROP_NOTIFY|CHAR_PROP_READ,
                           ATTR_PERMISSION_NONE,
                           GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP,
                           16, 0, &AccDataCharHandle);
  if (ret != BLE_STATUS_SUCCESS)
    return BLE_STATUS_ERROR;

  /* Fill the AccGyroMag BLE Characteristc */
  COPY_ACC_GYRO_MAG_W2ST_CHAR_UUID(uuid);
  BLUENRG_memcpy(&char_uuid.Char_UUID_128, uuid, 16);
  ret =  aci_gatt_add_char(HWServW2STHandle, UUID_TYPE_128, char_uuid.Char_UUID_128,
                           2+3*3*2,
						   CHAR_PROP_WRITE_WITHOUT_RESP,
                           ATTR_PERMISSION_NONE,
                           GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP,
                           16, 0, &AccGyroMagCharHandle);
  if (ret != BLE_STATUS_SUCCESS)
    return BLE_STATUS_ERROR;

  return BLE_STATUS_SUCCESS;
}

/**
 * @brief  Add the SW Feature service using a vendor specific profile
 * @param  None
 * @retval tBleStatus Status
 */
tBleStatus Add_SWServW2ST_Service(void)
{
  tBleStatus ret;
  int32_t NumberOfRecords=1;
  uint8_t uuid[16];

  COPY_SW_SENS_W2ST_SERVICE_UUID(uuid);
  BLUENRG_memcpy(&service_uuid.Service_UUID_128, uuid, 16);
  ret = aci_gatt_add_serv(UUID_TYPE_128, service_uuid.Service_UUID_128, PRIMARY_SERVICE,
                          1+3*NumberOfRecords, &SWServW2STHandle);

  if (ret != BLE_STATUS_SUCCESS) {
    goto fail;
  }

  COPY_QUATERNIONS_W2ST_CHAR_UUID(uuid);
  BLUENRG_memcpy(&char_uuid.Char_UUID_128, uuid, 16);
  ret =  aci_gatt_add_char(SWServW2STHandle, UUID_TYPE_128, char_uuid.Char_UUID_128,
                           2+6*SEND_N_QUATERNIONS,
                           CHAR_PROP_NOTIFY,
                           ATTR_PERMISSION_NONE,
                           GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP,
                           16, 0, &QuaternionsCharHandle);

  if (ret != BLE_STATUS_SUCCESS) {
    goto fail;
  }

  return BLE_STATUS_SUCCESS;

fail:
  return BLE_STATUS_ERROR;
}

/**
 * @brief  Update acceleration characteristic value
 * @param  AxesRaw_t structure containing acceleration value in mg.
 * @retval tBleStatus Status
 */
tBleStatus Acc_Update(AxesRaw_t *x_axes, AxesRaw_t *g_axes, AxesRaw_t *m_axes)
{
  uint8_t buff[2+2*3*3];
  tBleStatus ret;

  HOST_TO_LE_16(buff,(HAL_GetTick()>>3));

  HOST_TO_LE_16(buff+2,-x_axes->AXIS_Y);
  HOST_TO_LE_16(buff+4, x_axes->AXIS_X);
  HOST_TO_LE_16(buff+6,-x_axes->AXIS_Z);

  HOST_TO_LE_16(buff+8,g_axes->AXIS_Y);
  HOST_TO_LE_16(buff+10,g_axes->AXIS_X);
  HOST_TO_LE_16(buff+12,g_axes->AXIS_Z);

  HOST_TO_LE_16(buff+14,m_axes->AXIS_Y);
  HOST_TO_LE_16(buff+16,m_axes->AXIS_X);
  HOST_TO_LE_16(buff+18,m_axes->AXIS_Z);

  ret = aci_gatt_update_char_value(HWServW2STHandle, AccGyroMagCharHandle,
				   0, 2+2*3*3, buff);
  if (ret != BLE_STATUS_SUCCESS){
    PRINTF("Error while updating Acceleration characteristic: 0x%02X\n",ret) ;
    return BLE_STATUS_ERROR ;
  }

  return BLE_STATUS_SUCCESS;
}

tBleStatus Sample_rate_Update(uint8_t data)
{
	sample_rate = data;
}


tBleStatus ACC_DATA_Update(int d)
{
	if(d == 1){
		uint8_t buff = 3;
		printf("button\n");
	    aci_gatt_update_char_value(HWServW2STHandle, AccDataCharHandle, 0, 1, &buff);
	}

	else if(d == 2){
		uint8_t buff = 4;
		printf("button\n");
		aci_gatt_update_char_value(HWServW2STHandle, AccDataCharHandle, 0, 1, &buff);
	}

	else{
		int16_t pDataXYZ[3] = {0};
		BSP_ACCELERO_AccGetXYZ(pDataXYZ);

		// Store the new values in the buffers
		x_values[buffer_index] = pDataXYZ[0];
		y_values[buffer_index] = pDataXYZ[1];
		z_values[buffer_index] = pDataXYZ[2];

		/*char buffer[1000];
		snprintf(buffer, sizeof(buffer), "X: %d Y: %d Z: %d\n", pDataXYZ[0], pDataXYZ[1], pDataXYZ[2]);
		printf("sent: %s", buffer);*/

		// Compute the weighted moving average for the y values
		float processed_y = weighted_moving_average(y_values, buffer_index + 1);

		// Check the thresholds and send the result over Bluetooth
		check_thresholds_and_send(processed_y);

		// Update the buffer index
		buffer_index = (buffer_index + 1) % BUFFER_SIZE;

		return BLE_STATUS_SUCCESS;
	}

  /*int16_t pDataXYZ[3] = {0};


  BSP_ACCELERO_AccGetXYZ(pDataXYZ);
  int x = pDataXYZ[0], y = pDataXYZ[1], z = pDataXYZ[2];
  snprintf(buffer, sizeof(buffer), "X: %d Y: %d Z: %d\n", pDataXYZ[0], pDataXYZ[1], pDataXYZ[2]);*/
  //printf("sent: %s", buffer);
  //	  tBleStatus ret;
	  /*int16_t pDataXYZ[3] = {0};
	  BSP_ACCELERO_AccGetXYZ(pDataXYZ);


	  int x = pDataXYZ[0], y = pDataXYZ[1], z = pDataXYZ[2];*/

	  //	  tBleStatus ret;
	  /*uint8_t buff[6];
	  HOST_TO_LE_16(buff, x);

	  HOST_TO_LE_16(buff+2,y);
	  HOST_TO_LE_16(buff+4,z);

	  aci_gatt_update_char_value(HWServW2STHandle, AccDataCharHandle, 0, 6, buff);*/

    /*nprintf(buffer, sizeof(buffer), "X: %d Y: %d Z: %d\n", x, y, z);
    printf("sent: %s", buffer);*/

    //aci_gatt_update_char_value(HWServW2STHandle, AccDataCharHandle, 0, 6, buff);
  /*for(int i = 0; i < 64; i++){
	  snprintf(buffer, sizeof(buffer), "X: %d Y: %d Z: %d\n", bufferx[i], buffery[i], bufferz[i]);
	  printf("sent: %s", buffer);
	  uint8_t buff[6];
	  HOST_TO_LE_16(buff, (int)bufferx[i]);
	  HOST_TO_LE_16(buff+2,(int)buffery[i]);
	  HOST_TO_LE_16(buff+4,(int)bufferz[i]);
	  aci_gatt_update_char_value(HWServW2STHandle, AccDataCharHandle, 0, 6, buff);*/

	  /*printf("%d", bufferx[i]);
	  printf(",");*/
  //}
  //printf("\n");

}

/**
 * @brief  Update quaternions characteristic value
 * @param  SensorAxes_t *data Structure containing the quaterions
 * @retval tBleStatus      Status
 */
tBleStatus Quat_Update(AxesRaw_t *data)
{
  tBleStatus ret;
  uint8_t buff[2+6*SEND_N_QUATERNIONS];

  HOST_TO_LE_16(buff,(HAL_GetTick()>>3));

#if SEND_N_QUATERNIONS == 1
  HOST_TO_LE_16(buff+2,data[0].AXIS_X);
  HOST_TO_LE_16(buff+4,data[0].AXIS_Y);
  HOST_TO_LE_16(buff+6,data[0].AXIS_Z);
#elif SEND_N_QUATERNIONS == 2
  HOST_TO_LE_16(buff+2,data[0].AXIS_X);
  HOST_TO_LE_16(buff+4,data[0].AXIS_Y);
  HOST_TO_LE_16(buff+6,data[0].AXIS_Z);

  HOST_TO_LE_16(buff+8 ,data[1].AXIS_X);
  HOST_TO_LE_16(buff+10,data[1].AXIS_Y);
  HOST_TO_LE_16(buff+12,data[1].AXIS_Z);
#elif SEND_N_QUATERNIONS == 3
  HOST_TO_LE_16(buff+2,data[0].AXIS_X);
  HOST_TO_LE_16(buff+4,data[0].AXIS_Y);
  HOST_TO_LE_16(buff+6,data[0].AXIS_Z);

  HOST_TO_LE_16(buff+8 ,data[1].AXIS_X);
  HOST_TO_LE_16(buff+10,data[1].AXIS_Y);
  HOST_TO_LE_16(buff+12,data[1].AXIS_Z);

  HOST_TO_LE_16(buff+14,data[2].AXIS_X);
  HOST_TO_LE_16(buff+16,data[2].AXIS_Y);
  HOST_TO_LE_16(buff+18,data[2].AXIS_Z);
#else
#error SEND_N_QUATERNIONS could be only 1,2,3
#endif

  ret = aci_gatt_update_char_value(SWServW2STHandle, QuaternionsCharHandle,
				   0, 2+6*SEND_N_QUATERNIONS, buff);
  if (ret != BLE_STATUS_SUCCESS){
    PRINTF("Error while updating Sensor Fusion characteristic: 0x%02X\n",ret) ;
    return BLE_STATUS_ERROR ;
  }

  return BLE_STATUS_SUCCESS;
}

/*****************************ㄋ**************************************************
* Function Name  : Read_Request_CB.
* Description    : Update the sensor values.
* Input          : Handle of the characteristic to update.
* Return         : None.
*******************************************************************************/
void Read_Request_CB(uint16_t handle)
{
  tBleStatus ret;

  if(handle == AccGyroMagCharHandle + 1)
  {
    Acc_Update(&x_axes, &g_axes, &m_axes);
  }
  else if (handle == EnvironmentalCharHandle + 1)
  {
    float data_t, data_p;
    data_t = 27.0 + ((uint64_t)rand()*5)/RAND_MAX; //T sensor emulation
    data_p = 1000.0 + ((uint64_t)rand()*100)/RAND_MAX; //P sensor emulation
    BlueMS_Environmental_Update((int32_t)(data_p *100), (int16_t)(data_t * 10));
  }

  if(connection_handle !=0)
  {
    ret = aci_gatt_allow_read(connection_handle);
    if (ret != BLE_STATUS_SUCCESS)
    {
      PRINTF("aci_gatt_allow_read() failed: 0x%02x\r\n", ret);
    }
  }
}

tBleStatus BlueMS_Environmental_Update(int32_t press, int16_t temp)
{
  tBleStatus ret;
  uint8_t buff[8];
  HOST_TO_LE_16(buff, HAL_GetTick()>>3);

  HOST_TO_LE_32(buff+2,press);
  HOST_TO_LE_16(buff+6,temp);

  ret = aci_gatt_update_char_value(HWServW2STHandle, EnvironmentalCharHandle,
                                   0, 8, buff);

  if (ret != BLE_STATUS_SUCCESS){
    //PRINTF("Error while updating TEMP characteristic: 0x%04X\n",ret) ;
    return BLE_STATUS_ERROR ;
  }

  return BLE_STATUS_SUCCESS;
}
