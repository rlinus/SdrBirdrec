function [args] = getSdrDevice()
    
    args = [];

    format = @(str) strtrim(regexprep(str,'\n +',', '));
    toString = @(var) format(evalc(['disp(var)']));
    getStringList = @(list) cellfun(toString, list, 'UniformOutput', false);
    
    list = {};
    strlist = {};

    f = figure( 'Position',[1 1 700 310],...
                'MenuBar', 'none',...
                'Name', 'SDR Device Selector',...
                'NumberTitle', 'off');
    movegui(f,'center');

    listbox =   uicontrol('Style','listbox',...
                          'Position',[20 50 660 200],...
                          'String', strlist,...
                          'Visible', 'off', ...
                          'Max', 1);
                      
    loading_txt = uicontrol('Style','text',...
          'Position',[20 20 80 20],...
          'String','loading...', ...
          'Visible', 'off');
                      
    ok_btn = uicontrol('Style', 'pushbutton', ...
        'String', 'OK',...
        'Position', [620 20 60 20],...
        'Callback', @ok_btn_cb, ...
        'Enable', 'off');
    
    reload_btn = uicontrol('Style', 'pushbutton', ...
        'String', 'Reload',...
        'Position', [540 20 60 20],...
        'Callback', @reload_btn_cb);
    
    reload_btn_cb();
                      
    uiwait(f);
    if ishghandle(f)
        close(f);
    end

    function reload_btn_cb(hObject,callbackdata)
        ok_btn.Enable = 'off';
        listbox.Visible = 'off';
        loading_txt.Visible = 'on'; 
        drawnow;
        list = SdrBirdrecRecorder.findSdrDevices();
        strlist = getStringList(list);
        listbox.String = strlist;
        listbox.Visible = 'on';
        loading_txt.Visible = 'off';
        ok_btn.Enable = 'on';
    end

    function ok_btn_cb(hObject,callbackdata)
        args = list{listbox.Value};
        uiresume(hObject.Parent);
    end
                  
end
                  