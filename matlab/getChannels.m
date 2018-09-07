function [channel_table] = getChannels(channel_table, editable)
%getChannels get channel list from uitable
%   Detailed explanation goes here
if nargin == 0
    channel_table = {};
    editable = true;
end

VariableNames = {'channel_number','bird_name','min_transmitter_freq_MHz','max_transmitter_freq_MHz'};

if isempty(channel_table)
    e = [];
    channel_table = table(e,e,e,e,'VariableNames',VariableNames);
end

cell_table = table2cell(channel_table);

if editable
    enable = 'on';
else
    enable = 'off';
end


f = figure( 'Position',[1 1 490 310],...
            'MenuBar', 'none',...
            'Name', 'Radio channel list',...
            'NumberTitle', 'off');
movegui(f,'center');

uicontrol('Style','text',...
          'Position',[10 270 150 20],...
          'String','Number of radio channels:');
      
      
nrch_e =    uicontrol('Style','edit',...
                      'Position',[170 270 50 20],...
                      'String',num2str(height(channel_table)),...
                      'Enable', enable, ...
                      'Callback', @nrch_callback);
                  
t = uitable('Position',[20 50 441 200],...
            'RowName', [], ...
            'ColumnName', {'channel nr.','bird name','min. freq. [MHz]','max. freq. [MHz]'},...
            'ColumnFormat', {'numeric','char','numeric','numeric','numeric'}, ...
            'ColumnEditable', logical([0,1,1,1]),...
            'ColumnWidth', {70 150 110 110},...
            'Data', cell_table, ...
            'Enable', enable, ...
            'Tag', 'uichtable');
        
btn = uicontrol('Style', 'pushbutton', 'String', 'OK',...
        'Position', [215 20 60 20],...
        'Callback', @(hObject,callbackdata) check_table(hObject,callbackdata,t.Data));

    uiwait(f);
    if ishghandle(f)
        channel_table = cell2table(t.Data,'VariableNames',VariableNames);
        close(f);
    end
end


function nrch_callback(hObject,callbackdata)
    hdl = findobj(hObject.Parent, 'Tag','uichtable');
    
    table = hdl.Data;
    
    row = {0,'',0,0};
    
    nch = str2num(hObject.String);
    h = size(table,1);
    
    if nch<=h
        table = table(1:nch,:);
    else
        table(h+1:nch,:)=repmat(row,nch-h,1);
        table(:,1) = num2cell((0:nch-1)');
    end
    
    hdl.Data = table;
end

function check_table(hObject,callbackdata,table)
    v = cell2mat(table(:,3:4));
    
    if ~isempty(v) 
    
        if any(v(:,1)>v(:,2))
            errordlg('The min. frequencies must be smaller or equal the max. frequencies!');
            return;
        end

        v = sortrows(v)';

        if ~issorted(v(:))
            errordlg('The channels are not allowed to overlap!');
            return;
        end
    end
    
    uiresume(hObject.Parent);
end

