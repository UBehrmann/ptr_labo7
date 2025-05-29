cmake_minimum_required(VERSION 3.20)

project(labo6 C)

add_compile_options(-O2 -Wall -Wextra)

add_library(de1soc_utils STATIC de1soc_utils/de1soc_io.c 
	de1soc_utils/de1soc_video.c 
	de1soc_utils/de1soc_audio.c
	de1soc_utils/convolution.c
	de1soc_utils/fft_utils.c
	de1soc_utils/grayscale.c
	de1soc_utils/rtq.c)

add_executable(main main.c video_setup.c load_setup.c)

target_link_libraries(main pthread evl de1soc_utils m)