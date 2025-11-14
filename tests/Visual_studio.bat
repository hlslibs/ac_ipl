::This is a windows .bat file which runs the ac_ipl unit tests using the visual studio developer command prompt 

::Invoke the visual studio developer command prompt
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

::Set the path for Mgc_home
set MGC_HOME=C:\MGCNoScan\abeemana\sb\sif\ixn\Mgc_home

::All the unit tests
set SOURCES_CPP=rtest_ac_canny;^
  rtest_ac_ctc;^
  rtest_ac_dither;^
  rtest_ac_imhist;^
  rtest_ac_localcontrastnorm;^
  rtest_ac_gaussian_pyr;^
  rtest_ac_gamma;^
  rtest_ac_harris;^
  rtest_ac_dwt2_pyr;^
  rtest_ac_packed_vector;^
  rtest_ac_flag_gen;^
  rtest_ac_window_v2

::Compile and execute the unit tests
(for %%a in (%SOURCES_CPP%) do ( 
   cl /EHsc /I%MGC_HOME%\shared\include %%a.cpp
   %%a.exe
))
