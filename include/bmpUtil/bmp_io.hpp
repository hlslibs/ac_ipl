#pragma once
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

bool bmp_byte_swap_get ( void );
void bmp_byte_swap_set ( bool value );

typedef std::vector<unsigned char> bmp_io_vec_type;

bool check_vec_size ( const char* func_name, const char* vec_name,
                      const bmp_io_vec_type &vec, const std::size_t min_expected_size );

bool bmp_08_data_read ( std::ifstream &file_in, unsigned long int width, long int height, 
  unsigned char *rarray );
bool bmp_08_data_read ( std::ifstream &file_in, unsigned long int width, long int height, 
  bmp_io_vec_type &rvec );

void bmp_08_data_write ( std::ofstream &file_out, unsigned long int width, 
  long int height, unsigned char *rarray );
bool bmp_08_data_write ( std::ofstream &file_out, unsigned long int width, 
  long int height, const bmp_io_vec_type &rvec);

bool bmp_24_data_read ( std::ifstream &file_in, unsigned long int width, 
  long int height, unsigned char *rarray, unsigned char *garray, unsigned char *barray );
bool bmp_24_data_read ( std::ifstream &file_in, unsigned long int width, 
  long int height, bmp_io_vec_type &rvec, bmp_io_vec_type &gvec, bmp_io_vec_type &bvec );

void bmp_24_data_write ( std::ofstream &file_out, unsigned long int width, 
  long int height, unsigned char *rarray, unsigned char *garray, unsigned char *barray );
bool bmp_24_data_write ( std::ofstream &file_out, unsigned long int width, 
  long int height, bmp_io_vec_type &rvec, bmp_io_vec_type &gvec, bmp_io_vec_type &bvec );

void bmp_header1_print ( unsigned short int filetype, 
  unsigned long int filesize, unsigned short int reserved1, 
  unsigned short int reserved2, unsigned long int bitmapoffset );
bool bmp_header1_read ( std::ifstream &file_in, unsigned short int *filetype, 
  unsigned long int *filesize, unsigned short int *reserved1, 
  unsigned short int *reserved2, unsigned long int *bitmapoffset );
void bmp_header1_write ( std::ofstream &file_out, unsigned short int filetype,
  unsigned long int filesize, unsigned short int reserved1, 
  unsigned short int reserved2, unsigned long int bitmapoffset );

void bmp_header2_print ( unsigned long int size, unsigned long int width, 
  long int height, 
  unsigned short int planes, unsigned short int bitsperpixel, 
  unsigned long int compression, unsigned long int sizeofbitmap,
  unsigned long int horzresolution, unsigned long int vertresolution,
  unsigned long int colorsused,  unsigned long int colorsimportant );
bool bmp_header2_read ( std::ifstream &file_in, unsigned long int *size,
  unsigned long int *width, long int *height, 
  unsigned short int *planes, unsigned short int *bitsperpixel,
  unsigned long int *compression, unsigned long int *sizeofbitmap,
  unsigned long int *horzresolution, unsigned long int *vertresolution,
  unsigned long int *colorsused, unsigned long int *colorsimportant );
void bmp_header2_write ( std::ofstream &file_out, unsigned long int size,
  unsigned long int width, long int height, 
  unsigned short int planes, unsigned short int bitsperpixel,
  unsigned long int compression, unsigned long int sizeofbitmap,
  unsigned long int horzresolution, unsigned long int vertresolution,
  unsigned long int colorsused, unsigned long int colorsimportant );

void bmp_palette_print ( unsigned long int colorsused, 
  unsigned char *rparray, unsigned char *gparray, unsigned char *bparray,
  unsigned char *aparray );
bool bmp_palette_print ( unsigned long int colorsused, 
  bmp_io_vec_type &rpvec, bmp_io_vec_type &gpvec, bmp_io_vec_type &bpvec,
  bmp_io_vec_type &apvec );

bool bmp_palette_read ( std::ifstream &file_in, unsigned long int colorsused,
  unsigned char *rparray, unsigned char *gparray, unsigned char *bparray, 
  unsigned char *aparray );
bool bmp_palette_read ( std::ifstream &file_in, unsigned long int colorsused,
  bmp_io_vec_type &rpvec, bmp_io_vec_type &gpvec, bmp_io_vec_type &bpvec,
  bmp_io_vec_type &apvec );

void bmp_palette_write ( std::ofstream &file_out, unsigned long int colorsused, 
  unsigned char *rparray, unsigned char *gparray, unsigned char *bparray,
  unsigned char *aparray );
bool bmp_palette_write ( std::ofstream &file_out, unsigned long int colorsused, 
  bmp_io_vec_type &rpvec, bmp_io_vec_type &gpvec, bmp_io_vec_type &bpvec,
  bmp_io_vec_type &apvec );

bool bmp_print_test ( char *file_in_name );

bool bmp_get_dims ( char *file_in_name, unsigned long int *width, long int *height );

bool bmp_read ( char *file_in_name, unsigned long int *width, long int *height,
  unsigned char **rarray, unsigned char **garray, unsigned char **barray,
  bool internal_alloc = false ); // If internal_alloc = true, rarray, garray and barray are internally allocated.
bool bmp_read ( char *file_in_name, unsigned long int *width, long int *height, 
  bmp_io_vec_type &rvec, bmp_io_vec_type &gvec, bmp_io_vec_type &bvec );

bool bmp_read_test ( char *file_in_name );

bool bmp_08_write ( char *file_out_name, unsigned long int width,
  long int height, unsigned char *rarray );
bool bmp_08_write ( char *file_out_name, unsigned long int width,
  long int height, bmp_io_vec_type &rvec );

bool bmp_08_write_test ( char *file_out_name );

bool bmp_24_write ( char *file_out_name, unsigned long int width, long int height, 
  unsigned char *rarray, unsigned char *garray, unsigned char *barray );
bool bmp_24_write ( char *file_out_name, unsigned long int width, long int height, 
  bmp_io_vec_type &rvec, bmp_io_vec_type &gvec, bmp_io_vec_type &bvec );

bool bmp_24_write_test ( char *file_out_name );

bool long_int_read ( long int *long_int_val, std::ifstream &file_in );
void long_int_write ( long int long_int_val, std::ofstream &file_out );

bool u_long_int_read ( unsigned long int *u_long_int_val, std::ifstream &file_in );
void u_long_int_write ( unsigned long int u_long_int_val, std::ofstream &file_out );

bool u_short_int_read ( unsigned short int *u_short_int_val, std::ifstream &file_in );
void u_short_int_write ( unsigned short int u_short_int_val, std::ofstream &file_out );
