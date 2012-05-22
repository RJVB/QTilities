% cursorXYCallback - WindowButtonMotionFcn invoking a user-supplied
% callback with the current cursor position's (x,y) co-ordinates
%===============================================================================
% after:
% Author      : Rodney Thomson
%               http://iheartmatlab.blogspot.com
%===============================================================================                   
function cursorXYCallback(obj, event, callback, data)

    % find axes associated with calling window
    axes_handle = findobj(obj, 'Type', 'axes');
    
    % Only support figures with 1 axes (Avoids confusion)
    if (isempty(axes_handle) || (numel(axes_handle) > 1)) 
        % None or too many axes to draw into
        return;
    end
    
    curr_point      = get(axes_handle, 'CurrentPoint');
    position        = [curr_point(1, 1) curr_point(1, 2)];

    [x_lims y_lims in_bounds] = getBounds(axes_handle, position);
    
    if (~in_bounds)
        % Not in the plot bounds so dont update
        return;
	end
    
	callback( position, data );
end

%===============================================================================
% Description : Return the bounds of the supplied axes and identify whether
%               supplied position is contained within axes bounds
%===============================================================================
function [x_lims y_lims in_bounds] = getBounds(axes_handle, position)

    x_lims = get(axes_handle, 'XLim');
    y_lims = get(axes_handle, 'YLim');
    
    in_bounds = (position(1) >= x_lims(1)) && ...
                (position(1) <= x_lims(2)) && ...
                (position(2) >= y_lims(1)) && ...
                (position(2) <= y_lims(2));

end
