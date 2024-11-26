input = [-10, -9, -6,1,-15,-10,-7,-13,-10,-8,-10,-10,-9,-10,-9,-9,-10,-10,-10,-0,-6,-10,-10,-9,-10,-8,-11,-8,-9,-10,-10,-9,-11,-8,-11,-10,-40,48,235,533,-295,-625,-263,198,456,-226,-177,-165,-581,48,-635,-27,142,-685,-1042,-428,-1068,-34,-940,-73,-802,-924,-10,-144,299,-703,-472,-783,-1340,-220,-1616,774,-1863,878,-1865,-75,-1324,-582,-1171,282,-1770,249,-1819,-418,-1765,119,-1981,296,-1998,440,-1733,-428,-1789,-499,-1822]
size(input)
output = [0.018225, 0.032282, 0.025227,-0.029272,-0.088284,-0.138123,-0.092890,0.110213,0.387327,0.541158,0.38738,-0.416899,-1.666881,-3.195624,-4.711798,-6.000772,-7.015970,-7.848532,-8.605168,-9.306798,-9.872015,-10.172218,-10.166153,-9.926336,-9.608193,-9.379555, -9.316092, -9.406674, -9.549202,-9.650883,-9.619173,-9.440683,-9.162469,-8.941597,-8.833296,-8.887799,-9.016194,-9.32121, -9.911597,-10.869964,-9.695695,-6.591480,-3.394994,-3.450536,-10.662649,-23.609625, -35.414555,-31.869408,-2.245645,46.036804, 92.21205, 105.247490,70.712218, 2.179052, -66.721252, -102.292122, -90.503517, -55.399387, -32.747902, -54.383518, -115.012444, -185.105988, -233.204071, -26.270477, -253.339798, -286.930542, -369.470154, -485.211792, -592.005432,-655.293274, -658.928894,-746.488281,-788.163635, -765.048828, -685.889465, -605.501038]
size(output)
figure;
hold on;
plot(input, 'r', 'DisplayName', 'Input');
plot(output, 'b', 'DisplayName', 'Output');
hold off;

% Add labels and title
xlabel('Data Points');
ylabel('Valoues');
title('Input vs Output');
legend('show');
grid on;2j