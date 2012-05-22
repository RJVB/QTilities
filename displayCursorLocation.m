function displayCursorLocation(figure_handle, location, format_string, text_color)

	set(figure_handle, 'WindowButtonMotionFcn', ...
		@(obj, event)cursorLocation(obj, event, location, format_string, text_color));
end