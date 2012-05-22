function setMovieTimeFromCursorPos(position, movieAndDT)
	theMovie = movieAndDT{1}.Movie;
	t = position(1) - movieAndDT{2};
	if t > 0
		theMovie.time = t * theMovie.timeScale;
	else
		theMovie.time = 0
	end
end