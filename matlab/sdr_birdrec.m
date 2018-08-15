function varargout = sdr_birdrec(varargin)
% SDR_BIRDREC MATLAB code for sdr_birdrec.fig
%      SDR_BIRDREC, by itself, creates a new SDR_BIRDREC or raises the existing
%      singleton*.
%
%      H = SDR_BIRDREC returns the handle to a new SDR_BIRDREC or the handle to
%      the existing singleton*.
%
%      SDR_BIRDREC('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in SDR_BIRDREC.M with the given input arguments.
%
%      SDR_BIRDREC('Property','Value',...) creates a new SDR_BIRDREC or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before sdr_birdrec_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to sdr_birdrec_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help sdr_birdrec

% Last Modified by GUIDE v2.5 18-Jul-2018 14:23:25

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @sdr_birdrec_OpeningFcn, ...
                   'gui_OutputFcn',  @sdr_birdrec_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before sdr_birdrec is made visible.
function sdr_birdrec_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to sdr_birdrec (see VARARGIN)

% Choose default command line output for sdr_birdrec
handles.output = hObject;

% Update handles structure
guidata(hObject, handles);

% UIWAIT makes sdr_birdrec wait for user response (see UIRESUME)
% uiwait(handles.main_figure);

%init states
handles.sdr_device_btn.UserData.sdr_args = [];
handles.sdr_device_btn.UserData.sdrBirdrecBackend = SdrBirdrecBackend();
handles.channellist_btn.UserData.channel_list = {};

handles.sdr_samplerate_pm.String = {'2.4', '4.8', '6', '7.2', '9.6'};
handles.daqmx_terminalconfig_pm.String = {'DAQmx_Val_Cfg_Default', 'DAQmx_Val_RSE', 'DAQmx_Val_NRSE', 'DAQmx_Val_Diff', 'DAQmx_Val_PseudoDiff'};

%restore gui state
if exist('guistate.mat','file')
    load('guistate.mat');
    handles.folder_e.String = state.folder_e.String;
    handles.filename_e.String = state.filename_e.String;
    handles.split_files_cb.Value = state.split_files_cb.Value;
    handles.file_duration_e.String = state.file_duration_e.String;
    handles.sdrcenterfreq_e.String = state.sdrcenterfreq_e.String;
    handles.agc_cb.Value = state.agc_cb.Value;
    handles.tunergain_e.String = state.tunergain_e.String;
    handles.audiogain_e.String = state.audiogain_e.String;
    handles.squelchlevel_e.String = state.squelchlevel_e.String;
    handles.sg_scaling_e.String = state.sg_scaling_e.String;
    handles.sg_offset_e.String = state.sg_offset_e.String;
    handles.sdr_startontrigger_cb.Value = state.sdr_startontrigger_cb.Value;
    handles.sdr_externalclock_cb.Value = state.sdr_externalclock_cb.Value;
    handles.daqmx_nch_e.String = state.daqmx_nch_e.String;
    handles.daqmx_channellist_e.String = state.daqmx_channellist_e.String;
    handles.daqmx_terminalconfig_pm.Value = state.daqmx_terminalconfig_pm.Value;
    handles.daqmx_triggerterminal_e.String = state.daqmx_triggerterminal_e.String;
    handles.daqmx_externalclock_cb.Value = state.daqmx_externalclock_cb.Value;
    handles.daqmx_clockterminal_e.String = state.daqmx_clockterminal_e.String;
    handles.daqmx_antialiasing_cb.Value = state.daqmx_antialiasing_cb.Value;
    handles.daqmx_maxvoltage_e.String = state.daqmx_maxvoltage_e.String;
    handles.udp_cb.Value = state.udp_cb.Value;
    handles.udp_port_e.String = state.udp_port_e.String;
    handles.udp_ip_e.String = state.udp_ip_e.String;
    handles.sdr_samplerate_pm.Value = state.sdr_samplerate_pm.Value;
    
    handles.channellist_btn.UserData.channel_list = state.channellist_btn.UserData.channel_list;
    handles.sdr_device_btn.UserData.sdr_args = state.sdr_device_btn.UserData.sdr_args;
end

if handles.daqmx_externalclock_cb.Value
    handles.daqmx_clockterminal_e.Enable = 'on';
else
    handles.daqmx_clockterminal_e.Enable = 'off';
end

if handles.split_files_cb.Value
    handles.file_duration_e.Enable = 'on';
else
    handles.file_duration_e.Enable = 'off';
end
   
if handles.agc_cb.Value
    handles.tunergain_e.Enable = 'off';
else
    handles.tunergain_e.Enable = 'on';
end

if handles.udp_cb.Value
    handles.udp_port_e.Enable = 'on';
    handles.udp_ip_e.Enable = 'on';
else
    handles.udp_port_e.Enable = 'off';
    handles.udp_ip_e.Enable = 'off';
end

update_btn_group(handles);
    
global is_recording;
global monitor_settings

is_recording = false;
monitor_settings.play_audio = false;
monitor_settings.squelch_level = str2double(handles.squelchlevel_e.String);
monitor_settings.channel_type = 'sdr';
monitor_settings.channel_number = 0;

monitor_settings.show_spectrogram = false;
monitor_settings.spectrogram_scaling = str2double(handles.sg_scaling_e.String);
monitor_settings.spectrogram_offset = str2double(handles.sg_offset_e.String);


handles.radio_spectrum_axes.XLimMode = 'manual';
handles.radio_spectrum_axes.YLimMode = 'manual';
handles.radio_spectrum_axes.YLim = [-100 40];
handles.radio_spectrum_axes.YGrid = 'on';
handles.radio_spectrum_axes.XMinorTick = 'on';
ylabel(handles.radio_spectrum_axes,'dB');
xlabel(handles.radio_spectrum_axes,'MHz');

patch('Parent', handles.radio_spectrum_axes, 'XData',[],'YData',[],'CData',[], 'FaceAlpha', 0.5, 'EdgeColor', 'none', 'FaceColor', 'flat', 'Tag', 'patch_handle');


line('Parent', handles.radio_spectrum_axes, 'Tag', 'line_handle2', 'XData', [0 0], 'YData',[0 0], 'Color', 'r'); %create line in axes
line('Parent', handles.radio_spectrum_axes, 'Tag', 'line_handle', 'XData', [0 0], 'YData',[0 0]); %create line in axes
handles.spectrogram_axes.XLimMode = 'manual';
handles.spectrogram_axes.YLimMode = 'manual';
ylabel(handles.spectrogram_axes,'kHz');
xlabel(handles.spectrogram_axes,'time [s]');
image('Parent', handles.spectrogram_axes, 'Tag', 'spectrogram_handle', 'XData', [0 1], 'YData',[0 1], 'CData', zeros(500,240));
colormap(handles.spectrogram_axes,jet);



% --- Outputs from this function are returned to the command line.
function varargout = sdr_birdrec_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


function folder_e_Callback(hObject, eventdata, handles)
% hObject    handle to folder_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of folder_e as text
%        str2double(get(hObject,'String')) returns contents of folder_e as a double


% --- Executes during object creation, after setting all properties.
function folder_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to folder_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in folderselect_btn.
function folderselect_btn_Callback(hObject, eventdata, handles)
% hObject    handle to folderselect_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
folder_name = uigetdir;
if ischar(folder_name)
    handles.folder_e.String = folder_name;
end




function sdrcenterfreq_e_Callback(hObject, eventdata, handles)
% hObject    handle to sdrcenterfreq_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of sdrcenterfreq_e as text
%        str2double(get(hObject,'String')) returns contents of sdrcenterfreq_e as a double


% --- Executes during object creation, after setting all properties.
function sdrcenterfreq_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to sdrcenterfreq_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function transmitterfreqs_e_Callback(hObject, eventdata, handles)
% hObject    handle to transmitterfreqs_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of transmitterfreqs_e as text
%        str2double(get(hObject,'String')) returns contents of transmitterfreqs_e as a double


% --- Executes during object creation, after setting all properties.
function transmitterfreqs_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to transmitterfreqs_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function filename_e_Callback(hObject, eventdata, handles)
% hObject    handle to filename_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of filename_e as text
%        str2double(get(hObject,'String')) returns contents of filename_e as a double


% --- Executes during object creation, after setting all properties.
function filename_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to filename_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in startrec_btn.
function startrec_btn_Callback(hObject, eventdata, handles)
% hObject    handle to startrec_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global is_recording;

if is_recording
    return
end

settings.sdr_args = handles.sdr_device_btn.UserData.sdr_args;
settings.AGC = handles.agc_cb.Value;
settings.TunerGain = str2double(handles.tunergain_e.String);
settings.AudioGain = str2double(handles.audiogain_e.String);
settings.outputfolder = handles.folder_e.String;
settings.fn = handles.filename_e.String;
settings.split_files = handles.split_files_cb.Value;
settings.file_duration = max(str2double(handles.file_duration_e.String), 0.1);

settings.sdr_startontrigger = handles.sdr_startontrigger_cb.Value;
settings.sdr_externalclock = handles.sdr_externalclock_cb.Value;
settings.daqmx_nch = str2double(handles.daqmx_nch_e.String);
settings.daqmx_channellist = handles.daqmx_channellist_e.String;
settings.daqmx_terminalconfig = handles.daqmx_terminalconfig_pm.String{handles.daqmx_terminalconfig_pm.Value};
settings.daqmx_triggerterminal = handles.daqmx_triggerterminal_e.String;
settings.daqmx_externalclock = handles.daqmx_externalclock_cb.Value;
settings.daqmx_clockterminal = handles.daqmx_clockterminal_e.String;
settings.daqmx_antialiasing = handles.daqmx_antialiasing_cb.Value;
settings.daqmx_maxvoltage = str2double(handles.daqmx_maxvoltage_e.String);
settings.udp = handles.udp_cb.Value;
settings.udp_port = round(str2double(handles.udp_port_e.String));
settings.udp_ip = handles.udp_ip_e.String;

settings.sdr_sample_rate_index = handles.sdr_samplerate_pm.Value;
settings.sdr_center_freq = str2double(handles.sdrcenterfreq_e.String);
settings.channel_list = handles.channellist_btn.UserData.channel_list;

% for i=1:length(gui_settings.transmitter_freqs)
%     gui_settings.full_fns{i} = [gui_settings.outputfolder '\' gui_settings.fn '_ch' num2str(i)];
%     if exist([gui_settings.full_fns{i} '.mat'],'file')
%         choice = questdlg(sprintf('The file %s already exists. Do you want to overwrite it?',gui_settings.full_fns{i}),'Warning!', 'Yes', 'No', 'No');
%         if strcmpi(choice,'No')
%             return;
%         end
%     end
% end

% if any(any(gui_settings.channel_list{:,3:4}<gui_settings.sdr_center_freq-fs_hf/2)) || any(any(gui_settings.channel_list{:,3:4}>gui_settings.sdr_center_freq+fs_hf/2))
%     errordlg('Some frequencies in the channel list are out of range!');
%     return;
% end

if isempty(settings.sdr_args)
    errordlg('No SDR device is selected.')
    return;
end

% gui_settings.full_fn = [gui_settings.outputfolder '\' gui_settings.fn '.mat'];
% if exist(gui_settings.full_fn,'file')
%     choice = questdlg(sprintf('The file %s already exists. Do you want to overwrite it?',gui_settings.fn),'Warning!', 'Yes', 'No', 'No');
%     if strcmpi(choice,'No')
%         return;
%     end
% end

hObject.Enable = 'off';
handles.stoprec_btn.Enable = 'on';


handles_struct = guihandles(handles.radio_spectrum_axes);
handles_struct2 = guihandles(handles.spectrogram_axes);

gui_handles.status_txt = handles.status_txt;
gui_handles.pll_txt = handles.pll_txt;
gui_handles.patch_handle = handles_struct.patch_handle;
gui_handles.line_handle = handles_struct.line_handle;
gui_handles.line_handle2 = handles_struct.line_handle2;
gui_handles.spectrogram_handle = handles_struct2.spectrogram_handle;

drawnow;

try
    recorder(handles.sdr_device_btn.UserData.sdrBirdrecBackend, settings,gui_handles);

    hObject.Enable = 'on';
    handles.stoprec_btn.Enable = 'off';
    handles.main_figure.UserData = settings;
catch ME
    is_recording = false;
    hObject.Enable = 'on';
    handles.stoprec_btn.Enable = 'off';
    handles.main_figure.UserData = settings;
    handles.status_txt.String = 'Error occured! Start a new recording.';
    ME.rethrow;
end


% --- Executes on button press in stoprec_btn.
function stoprec_btn_Callback(hObject, eventdata, handles)
% hObject    handle to stoprec_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global is_recording;
is_recording = false;

    


function tunergain_e_Callback(hObject, eventdata, handles)
% hObject    handle to tunergain_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of tunergain_e as text
%        str2double(get(hObject,'String')) returns contents of tunergain_e as a double


% --- Executes during object creation, after setting all properties.
function tunergain_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to tunergain_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in play_btn.
function play_btn_Callback(hObject, eventdata, handles)
% hObject    handle to play_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global monitor_settings;
monitor_settings.play_audio = true;
h = handles.sdr_device_btn.UserData.sdrBirdrecBackend;
if ~isempty(h) && h.isStreaming()
    h.setPlayAudio(true);
end

% --- Executes on button press in stop_btn.
function stop_btn_Callback(hObject, eventdata, handles)
% hObject    handle to stop_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global monitor_settings;
monitor_settings.play_audio = false;
h = handles.sdr_device_btn.UserData.sdrBirdrecBackend;
if ~isempty(h) && h.isStreaming()
    h.setPlayAudio(false);
end


% --- Executes during object deletion, before destroying properties.
function main_figure_DeleteFcn(hObject, eventdata, handles)
% hObject    handle to main_figure (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global is_recording;
is_recording = false;


% --- Executes when user attempts to close main_figure.
function main_figure_CloseRequestFcn(hObject, eventdata, handles)
% hObject    handle to main_figure (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: delete(hObject) closes the figure
global is_recording;

if is_recording
    warndlg('Stop the recording before you close this window!');
    %choice = questdlg(sprintf('You are recording?',gui_settings.full_fns{i}),'Warning!', 'Yes', 'No', 'No');
    return;
end


%save gui state
try
path = fileparts(mfilename('fullpath'));
state.folder_e.String = handles.folder_e.String;
state.filename_e.String = handles.filename_e.String;
state.split_files_cb.Value = handles.split_files_cb.Value;
state.file_duration_e.String = handles.file_duration_e.String;
state.sdrcenterfreq_e.String = handles.sdrcenterfreq_e.String;
state.agc_cb.Value = handles.agc_cb.Value;
state.tunergain_e.String = handles.tunergain_e.String;
state.audiogain_e.String = handles.audiogain_e.String;
state.squelchlevel_e.String = handles.squelchlevel_e.String;
state.sg_scaling_e.String = handles.sg_scaling_e.String;
state.sg_offset_e.String = handles.sg_offset_e.String;
state.sdr_startontrigger_cb.Value = handles.sdr_startontrigger_cb.Value;
state.sdr_externalclock_cb.Value = handles.sdr_externalclock_cb.Value;
state.daqmx_nch_e.String = handles.daqmx_nch_e.String;
state.daqmx_channellist_e.String = handles.daqmx_channellist_e.String;
state.daqmx_terminalconfig_pm.Value = handles.daqmx_terminalconfig_pm.Value;
state.daqmx_triggerterminal_e.String = handles.daqmx_triggerterminal_e.String;
state.daqmx_externalclock_cb.Value = handles.daqmx_externalclock_cb.Value;
state.daqmx_clockterminal_e.String = handles.daqmx_clockterminal_e.String;
state.daqmx_antialiasing_cb.Value = handles.daqmx_antialiasing_cb.Value;
state.daqmx_maxvoltage_e.String = handles.daqmx_maxvoltage_e.String;
state.udp_cb.Value = handles.udp_cb.Value;
state.udp_port_e.String = handles.udp_port_e.String;
state.udp_ip_e.String = handles.udp_ip_e.String;
state.sdr_samplerate_pm.Value = handles.sdr_samplerate_pm.Value;

state.channellist_btn.UserData.channel_list = handles.channellist_btn.UserData.channel_list;
state.sdr_device_btn.UserData.sdr_args = handles.sdr_device_btn.UserData.sdr_args;


save([path '/guistate.mat'],'state');


end


delete(hObject);



function audiogain_e_Callback(hObject, eventdata, handles)
% hObject    handle to audiogain_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of audiogain_e as text
%        str2double(get(hObject,'String')) returns contents of audiogain_e as a double


% --- Executes during object creation, after setting all properties.
function audiogain_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to audiogain_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in agc_cb.
function agc_cb_Callback(hObject, eventdata, handles)
% hObject    handle to agc_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of agc_cb

if handles.agc_cb.Value
    handles.tunergain_e.Enable = 'off';
else
    handles.tunergain_e.Enable = 'on';
end



function squelchlevel_e_Callback(hObject, eventdata, handles)
% hObject    handle to squelchlevel_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of squelchlevel_e as text
%        str2double(get(hObject,'String')) returns contents of squelchlevel_e as a double
global monitor_settings;
monitor_settings.squelch_level = str2double(handles.squelchlevel_e.String);
h = handles.sdr_device_btn.UserData.sdrBirdrecBackend;
if ~isempty(h) && h.isStreaming()
    h.setSquelch(str2double(handles.squelchlevel_e.String));
end

% --- Executes during object creation, after setting all properties.
function squelchlevel_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to squelchlevel_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in show_btn.
function show_btn_Callback(hObject, eventdata, handles)
% hObject    handle to show_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global monitor_settings;
monitor_settings.show_spectrogram = true;


% --- Executes on button press in stopspec_btn.
function stopspec_btn_Callback(hObject, eventdata, handles)
% hObject    handle to stopspec_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global monitor_settings;
monitor_settings.show_spectrogram = false;


function daqmx_maxvoltage_e_Callback(hObject, eventdata, handles)
% hObject    handle to daqmx_maxvoltage_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of daqmx_maxvoltage_e as text
%        str2double(get(hObject,'String')) returns contents of daqmx_maxvoltage_e as a double


% --- Executes during object creation, after setting all properties.
function daqmx_maxvoltage_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to daqmx_maxvoltage_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end

% --- Executes on button press in udp_cb.
function udp_cb_Callback(hObject, eventdata, handles)
% hObject    handle to udp_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of udp_cb
if handles.udp_cb.Value
    handles.udp_port_e.Enable = 'on';
    handles.udp_ip_e.Enable = 'on';
else
    handles.udp_port_e.Enable = 'off';
    handles.udp_ip_e.Enable = 'off';
end


function udp_port_e_Callback(hObject, eventdata, handles)
% hObject    handle to udp_port_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of udp_port_e as text
%        str2double(get(hObject,'String')) returns contents of udp_port_e as a double


% --- Executes during object creation, after setting all properties.
function udp_port_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to udp_port_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function udp_ip_e_Callback(hObject, eventdata, handles)
% hObject    handle to udp_ip_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of udp_ip_e as text
%        str2double(get(hObject,'String')) returns contents of udp_ip_e as a double


% --- Executes during object creation, after setting all properties.
function udp_ip_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to udp_ip_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end

% --- Executes during object creation, after setting all properties.
function agc_cb_CreateFcn(hObject, eventdata, handles)
% hObject    handle to agc_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% --- Executes during object creation, after setting all properties.
function daqmx_antialiasing_cb_CreateFcn(hObject, eventdata, handles)
% hObject    handle to agc_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% --- Executes during object creation, after setting all properties.
function udp_cb_CreateFcn(hObject, eventdata, handles)
% hObject    handle to agc_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called


% --- Executes on button press in channellist_btn.
function channellist_btn_Callback(hObject, eventdata, handles)
% hObject    handle to channellist_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global is_recording;
global monitor_settings;
hObject.UserData.channel_list = getChannels(hObject.UserData.channel_list,~is_recording);

if ~is_recording
    update_btn_group(handles);
end



function sg_scaling_e_Callback(hObject, eventdata, handles)
% hObject    handle to sg_scaling_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of sg_scaling_e as text
%        str2double(get(hObject,'String')) returns contents of sg_scaling_e as a double
global monitor_settings;
monitor_settings.spectrogram_scaling = str2double(handles.sg_scaling_e.String);


% --- Executes during object creation, after setting all properties.
function sg_scaling_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to sg_scaling_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function sg_offset_e_Callback(hObject, eventdata, handles)
% hObject    handle to sg_offset_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of sg_offset_e as text
%        str2double(get(hObject,'String')) returns contents of sg_offset_e as a double
global monitor_settings;
monitor_settings.spectrogram_offset = str2double(handles.sg_offset_e.String);


% --- Executes during object creation, after setting all properties.
function sg_offset_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to sg_offset_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes during object creation, after setting all properties.
function channels_btn_group_CreateFcn(hObject, eventdata, handles)
% hObject    handle to channels_btn_group (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called


function update_btn_group(handles)

sdrChannelCount = size(handles.channellist_btn.UserData.channel_list,1);
for i=0:sdrChannelCount-1
    handles.(sprintf('pb_ch%i_tb',i)).Enable = 'on';
end
for i=sdrChannelCount:7
    handles.(sprintf('pb_ch%i_tb',i)).Enable = 'off';
end

daqmxChannelCount = str2double(handles.daqmx_nch_e.String);
for i=0:daqmxChannelCount-1
    handles.(sprintf('daqmx_ch%i_tb',i)).Enable = 'on';
end
for i=daqmxChannelCount:7
    handles.(sprintf('daqmx_ch%i_tb',i)).Enable = 'off';
end

% --- Executes when selected object is changed in channels_btn_group.
function channels_btn_group_SelectionChangedFcn(hObject, eventdata, handles)
% hObject    handle to the selected object in channels_btn_group 
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global monitor_settings

h = handles.sdr_device_btn.UserData.sdrBirdrecBackend;
if ~isempty(h) && h.isStreaming()
    if strfind(handles.channels_btn_group.SelectedObject.Tag, 'pb')
        monitor_settings.channel_type = 'sdr';
    elseif strfind(handles.channels_btn_group.SelectedObject.Tag, 'daqmx')
        monitor_settings.channel_type = 'daqmx';
    else
        error('invalid tag');
    end
    monitor_settings.channel_number = str2double(handles.channels_btn_group.SelectedObject.String);
    h.setChannel(monitor_settings.channel_type, monitor_settings.channel_number);
end


% --- Executes on button press in sdr_device_btn.
function sdr_device_btn_Callback(hObject, eventdata, handles)
% hObject    handle to sdr_device_btn (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
hObject.Enable = 'off';
global is_recording;
global monitor_settings;
hObject.UserData.sdr_args = getSdrDevice();

hObject.Enable = 'on';


% --- Executes on selection change in sdr_samplerate_pm.
function sdr_samplerate_pm_Callback(hObject, eventdata, handles)
% hObject    handle to sdr_samplerate_pm (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: contents = cellstr(get(hObject,'String')) returns sdr_samplerate_pm contents as cell array
%        contents{get(hObject,'Value')} returns selected item from sdr_samplerate_pm


% --- Executes during object creation, after setting all properties.
function sdr_samplerate_pm_CreateFcn(hObject, eventdata, handles)
% hObject    handle to sdr_samplerate_pm (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: popupmenu controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in sdr_startontrigger_cb.
function sdr_startontrigger_cb_Callback(hObject, eventdata, handles)
% hObject    handle to sdr_startontrigger_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of sdr_startontrigger_cb


% --- Executes on button press in daqmx_antialiasing_cb.
function daqmx_antialiasing_cb_Callback(hObject, eventdata, handles)
% hObject    handle to daqmx_antialiasing_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of daqmx_antialiasing_cb



function daqmx_nch_e_Callback(hObject, eventdata, handles)
% hObject    handle to daqmx_nch_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of daqmx_nch_e as text
%        str2double(get(hObject,'String')) returns contents of daqmx_nch_e as a double
global is_recording;

if ~is_recording
    update_btn_group(handles);
end

% --- Executes during object creation, after setting all properties.
function daqmx_nch_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to daqmx_nch_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function daqmx_channellist_e_Callback(hObject, eventdata, handles)
% hObject    handle to daqmx_channellist_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of daqmx_channellist_e as text
%        str2double(get(hObject,'String')) returns contents of daqmx_channellist_e as a double


% --- Executes during object creation, after setting all properties.
function daqmx_channellist_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to daqmx_channellist_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on selection change in daqmx_terminalconfig_pm.
function daqmx_terminalconfig_pm_Callback(hObject, eventdata, handles)
% hObject    handle to daqmx_terminalconfig_pm (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: contents = cellstr(get(hObject,'String')) returns daqmx_terminalconfig_pm contents as cell array
%        contents{get(hObject,'Value')} returns selected item from daqmx_terminalconfig_pm


% --- Executes during object creation, after setting all properties.
function daqmx_terminalconfig_pm_CreateFcn(hObject, eventdata, handles)
% hObject    handle to daqmx_terminalconfig_pm (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: popupmenu controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function daqmx_triggerterminal_e_Callback(hObject, eventdata, handles)
% hObject    handle to daqmx_triggerterminal_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of daqmx_triggerterminal_e as text
%        str2double(get(hObject,'String')) returns contents of daqmx_triggerterminal_e as a double


% --- Executes during object creation, after setting all properties.
function daqmx_triggerterminal_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to daqmx_triggerterminal_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end



function daqmx_clockterminal_e_Callback(hObject, eventdata, handles)
% hObject    handle to daqmx_clockterminal_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of daqmx_clockterminal_e as text
%        str2double(get(hObject,'String')) returns contents of daqmx_clockterminal_e as a double


% --- Executes during object creation, after setting all properties.
function daqmx_clockterminal_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to daqmx_clockterminal_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in sdr_externalclock_cb.
function sdr_externalclock_cb_Callback(hObject, eventdata, handles)
% hObject    handle to sdr_externalclock_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of sdr_externalclock_cb


% --- Executes on button press in split_files_cb.
function split_files_cb_Callback(hObject, eventdata, handles)
% hObject    handle to split_files_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of split_files_cb
if handles.split_files_cb.Value
    handles.file_duration_e.Enable = 'on';
else
    handles.file_duration_e.Enable = 'off';
end


function file_duration_e_Callback(hObject, eventdata, handles)
% hObject    handle to file_duration_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of file_duration_e as text
%        str2double(get(hObject,'String')) returns contents of file_duration_e as a double


% --- Executes during object creation, after setting all properties.
function file_duration_e_CreateFcn(hObject, eventdata, handles)
% hObject    handle to file_duration_e (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in daqmx_externalclock_cb.
function daqmx_externalclock_cb_Callback(hObject, eventdata, handles)
% hObject    handle to daqmx_externalclock_cb (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of daqmx_externalclock_cb
if handles.daqmx_externalclock_cb.Value
    handles.daqmx_clockterminal_e.Enable = 'on';
else
    handles.daqmx_clockterminal_e.Enable = 'off';
end
