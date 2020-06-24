function SerialPorts_Init(McuPort)
global P;
delete(instrfindall('Type','serial'));         % delete the last configuration for serial ports

McuSerial = instrfind('Type','serial','Port',McuPort,'Tag','');  % find if there have serial port for MCU port
if isempty(McuSerial)                          % if there do not have one
    McuSerial = serial(McuPort);               % create one
else                                           % if there exists one
    fclose(McuSerial);                         
    McuSerial = McuSerial(1);                  % use the first one
end
% set the properties of the serial port
McuSerial.BaudRate = 115200;
McuSerial.InputBufferSize = 100000;
McuSerial.OutputBufferSize = 512;
McuSerial.Terminator = 'CR/LF';
McuSerial.DataTerminalReady='on';
McuSerial.RequestToSend='off';
% store the serial port configuration
P.config{1,1} = McuSerial;
% try to open the assigned MCU&PC communication port
try
    fopen(McuSerial);
    fscanf(McuSerial);
    disp('MCU COM opened !');
catch
    msgbox('Can not open MCU COM !');
    return
end


end