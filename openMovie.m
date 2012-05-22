% open the movie <URL> in a new window, attempting to adapt the window size
% to the movie size. If <dataFigure> is given, additional calls are made
% such that the movie is spooled to the frame corresponding to the time  at
% the cursor in <dataFigure>, possibly taking into account a start delay of
% the video (movie) w.r.t. the data shown.

function [win movieCtrl] = openMovie( URL, dataFigure, startDelay )
	screen = get(0, 'ScreenSize');
	win=figure('position', [0 0 1080 630]);
	set( win, 'Visible', 'off');
	movieCtrl=actxcontrol('QTOControl.QTControl.1', [0 0 1080 630], win);
	movieCtrl.QuickTimeInitialize();
	movieCtrl.URL=URL;
	movieCtrl.MovieResizingUnlock()
	movieCtrl.Movie.AllowDynamicResize = 1;
	movieCtrl.Movie.SeeAllFrames=1;
	h = movieCtrl.Movie.Height + 16; % movie controller height
	set( win, 'BackingStore', 'on' );
	set( win, 'Position', [0, screen(4)-h-60, movieCtrl.Movie.Width, h] );
	set( win, 'Visible', 'on');
	movieCtrl.SetScale(1)
	movieCtrl.Movie.UpdateMovie()
	movieCtrl.Movie.Draw()
	if nargin >= 2
		if nargin == 2
			startDelay = 0
		end
		installCursorXYCallback(dataFigure, @setMovieTimeFromCursorPos, {movieCtrl startDelay});
	end
end
