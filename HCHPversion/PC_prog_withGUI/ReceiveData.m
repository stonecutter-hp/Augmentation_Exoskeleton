function ReceiveData(obj,~)
global P;
%INSTRCALLBACK Display event information for the event.
%
%   INSTRCALLBACK(OBJ, EVENT) displays a message which contains the 
%   type of the event, the time of the event and the name of the
%   object which caused the event to occur.  
%
%   If an error event occurs, the error message is also displayed.  
%   If a pin event occurs, the pin that changed value and the pin 
%   value is also displayed. 
%
%   INSTRCALLBACK is an example callback function. Use this callback 
%   function as a template for writing your own callback function.
%
%   Example:
%       s = serial('COM1');
%       set(s, 'OutputEmptyFcn', {'instrcallback'});
%       fopen(s);
%       fprintf(s, '*IDN?', 'async');
%       idn = fscanf(s);
%       fclose(s);
%       delete(s);
%

%   MP 2-24-00
%   Copyright 1999-2011 The MathWorks, Inc. 
%   $Revision: 1.1.6.2 $  $Date: 2011/05/13 18:06:14 $


% switch nargin
% case 0
%    error(message('instrument:instrument:instrcallback:invalidSyntaxArgv'));
% case 1
%    error(message('instrument:instrument:instrcallback:invalidSyntax'));
% case 2
%    if ~isa(obj, 'instrument') || ~isa(event, 'struct')
%       error(message('instrument:instrument:instrcallback:invalidSyntax'));
%    end   
%    if ~(isfield(event, 'Type') && isfield(event, 'Data'))
%       error(message('instrument:instrument:instrcallback:invalidSyntax'));
%    end
% end  
% 
% % Determine the type of event.
% EventType = event.Type;
% 
% % Determine the time of the error event.
% EventData = event.Data;
% EventDataTime = EventData.AbsTime;
%    
% % Create a display indicating the type of event, the time of the event and
% % the name of the object.
% name = get(obj, 'Name');
% fprintf([EventType ' event occurred at ' datestr(EventDataTime,13),...
% 	' for the object: ' name '.\n']);
% 
% % Display the error string.
% if strcmpi(EventType, 'error')
% 	fprintf([EventData.Message '\n']);
% end
% 
% % Display the pinstatus information.
% if strcmpi(EventType, 'pinstatus')
%     fprintf([EventData.Pin ' is ''' EventData.PinValue '''.\n']);
% end
% 
% % Display the trigger line information.
% if strcmpi(EventType, 'trigger')
%     fprintf(['The trigger line is ' EventData.TriggerLine '.\n']);
% end
% 
% % Display the datagram information.
% if strcmpi(EventType, 'datagramreceived')
%     fprintf([num2str(EventData.DatagramLength) ' bytes were ',...
%             'received from address ' EventData.DatagramAddress,...
%             ', port ' num2str(EventData.DatagramPort) '.\n']);
% end

% Display the configured value information.
% if strcmpi(EventType, 'confirmation')
%     fprintf([EventData.PropertyName ' was configured to ' num2str(EventData.ConfiguredValue) '.\n']);
% end


P.TimeAll = [P.TimeAll toc];
%% Read data from MCU
TransState = fscanf(obj);   % read the input buffer
% flushinput(obj);
% ensure the data completeness
% while numel(TransState) ~= P.ReceiveDataNum
% % flushinput(obj);
%     TransState = fscanf(obj);
% end
TransTest = (TransState(1)-48)*1000 + (TransState(2)-48)*100 +...
            (TransState(3)-48)*10 + (TransState(4)-48)*1;
% TransTest = str2double(TransState);
% if(P.Send_Update == 0)
    P.test = [P.test, TransTest];
    P.Send_Update = 1;
% end

% After receiving, allow next control and send cycle
% P.Control_Update = 1;
end