/**
 * FujiNet for #Adam configuration program
 *
 * Select file from Host Slot
 */

#include <string.h>
#include "fuji_typedefs.h"
#include "screen.h"
#include "io.h"
#include "select_file.h"
#include "globals.h"
#include "input.h"
#include "bar.h"

#define ENTRIES_PER_PAGE 15

char path[224];
char filter[32];
DirectoryPosition pos=0;
bool dir_eof=false;
bool quick_boot=false;

static enum
  {
   INIT,
   DISPLAY,
   NEXT_PAGE,
   PREV_PAGE,
   CHOOSE,
   ADVANCE_FOLDER,
   DEVANCE_FOLDER,
   DONE
  } subState;

void select_file_init(void)
{
  pos=0;
  memset(path,0,256);
  memset(filter,0,32);
  screen_select_file();
  subState=DISPLAY;
  quick_boot=dir_eof=false;
}

unsigned char select_file_display(void)
{
  char visibleEntries=0;
  
  io_mount_host_slot(selected_host_slot);

  if (io_error())
    {
      screen_error("  COULD NOT MOUNT HOST SLOT.");
      subState=DONE;
      state=SET_WIFI;
      return;
    }

  screen_select_file_display(path,filter);
  
  io_open_directory(selected_host_slot,path,filter);
  
  if (io_error())
    {
      screen_error("  COULD NOT OPEN DIRECTORY.");
      subState=DONE;
      state=SET_WIFI;
      return;
    }
  
  if (pos>0)
    io_set_directory_position(pos);
  
  for (char i=0;i<ENTRIES_PER_PAGE;i++)
    {
      char *e = io_read_directory(31,0);
      if (e[1]==0x7F)
	{
	  dir_eof=true;
	  break;
	}
      else
	{
	  visibleEntries++;
	  screen_select_file_display_entry(i,e);
	}
    }
  
  io_close_directory();
  subState=CHOOSE;
  return visibleEntries;
}

void select_next_page(void)
{
  pos += ENTRIES_PER_PAGE;
  subState=DISPLAY;
  dir_eof=false;
}

void select_prev_page(void)
{
  pos -= ENTRIES_PER_PAGE;
  subState=DISPLAY;
  dir_eof=false;
}

void select_file_choose(char visibleEntries)
{
  char k=0;
  
  screen_select_file_choose(visibleEntries);

  while (subState==CHOOSE)
    {
      k=input();
      switch(k)
	{
	case 0x0d:
	  pos+=bar_get();
	  subState=DONE;
	  break;
	case 0x1b:
	  subState=DONE;
	  state=HOSTS_AND_DEVICES;
	  break;
	case 0x84:
	  state=DEVANCE_FOLDER;
	  break;
	case 0x85:
	  break;
	case 0x86:
	  quick_boot=true;
	  pos+=bar_get();
	  subState=DONE;
	  state=SELECT_SLOT;
	  break;
	case 0xA0:
	  if ((bar_get() == 0) && (pos > 0))
	    {
	      subState=PREV_PAGE;
	    }
	  else
	    bar_up();
	  break;
	case 0xA2:
	  if ((bar_get() == 14) && (dir_eof==false))
	    {
	      subState=NEXT_PAGE;
	    }
	  else
	    bar_down();
	  break;
	}
    }
}

void select_file_advance(void)
{
  char *e;
  
  io_open_directory(selected_host_slot,path,filter);

  io_set_directory_position(pos);

  e = io_read_directory(256,1);
  
  io_close_directory();

  strcat(path,e); // append directory entry to end of current path

  subState=DISPLAY; // and display the result.
}

void select_file_devance(void)
{
  char *p = strrchr(path,'/'); // find end of directory string (last /)
  
  while (*--p != '/'); // scoot backward until we reach next /

  p++; // scoot forward 

  *p = 0; // truncate string.
  
  subState=DISPLAY; // And display the result.
}

bool select_file_is_folder(void)
{
  char *e;
  
  io_open_directory(selected_host_slot,path,filter);

  io_set_directory_position(pos);

  e = io_read_directory(31,1);
  
  io_close_directory();

  return e[10]; // Offset 10 = directory flag.
}

void select_file_done(void)
{
  if (select_file_is_folder())
      subState=ADVANCE_FOLDER;
  else
    state=SELECT_SLOT;
}

void select_file(void)
{
  char visibleEntries=0;

  subState=INIT;
  
  while (state==SELECT_FILE)
    {
      switch(subState)
	{
	case INIT:
	  select_file_init();
	  break;
	case DISPLAY:
	  visibleEntries=select_file_display();
	  break;
	case NEXT_PAGE:
	  select_next_page();
	  break;
	case PREV_PAGE:
	  select_prev_page();
	  break;
	case CHOOSE:
	  select_file_choose(visibleEntries);
	  break;
	case ADVANCE_FOLDER:
	  select_file_advance();
	  break;
	case DEVANCE_FOLDER:
	  select_file_devance();
	  break;
	case DONE:
	  select_file_done();
	  break;
	}
    }
}
