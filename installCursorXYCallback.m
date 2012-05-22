% installCursorXYCallback - installs a WindowButtonMotionFcn invoking a user-supplied
% callback with the current cursor position's (x,y) co-ordinates
%
% example call:
% installCursorXYCallback(f, @setMovieTimeFromCursorPos, {kk videoStartDelay})
% where kk is a handle to a QTOControl.QTControl object returned by
% actxcontrol(), in which a movie is opened. videoStartDelay is the time
% delay of the video recording start w.r.t. the start of the data in figure
% <figure_handle>.
function installCursorXYCallback(figure_handle, callback, data)

	set(figure_handle, 'WindowButtonMotionFcn', ...
		@(obj, event)cursorXYCallback(obj, event, callback, data));
end