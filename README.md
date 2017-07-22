# CSplitMovie
This is a tools to split a movie based on black screens or white screens.
The development environment is visual studio 2013 community with opencv 2.4.10.<br>
<br>
This is a simple tool while all codes are stored inside CSplitMovie.cpp. 
To use this tool, you need to have libraries of opencv, ffmpeg.exe and gnuplot.
Usage is simple, just hit `CSplitMovie.exe fileName`, then a series of .bat files 
which contains splitting commands with different time slots and a .png file showing 
the power of every frames of the movie  will be generated. Based on the information provided by the
.png file to choose the best time slots setting by the user should be the final step.<br>
<br>
