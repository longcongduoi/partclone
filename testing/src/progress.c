/**
 * progress.c - part of Partclone project
 *
 * Copyright (c) 2007~ Thomas Tsai <thomas at nchc org tw>
 *
 * progress bar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "config.h"
#include "progress.h"
#include "gettext.h"
#define _(STRING) gettext(STRING)
#include "partclone.h"

#ifdef HAVE_LIBNCURSESW
    #include <ncurses.h>
    extern WINDOW *p_win;
    extern WINDOW *bar_win;
    int window_f = 0;
    int color_support = 1;
#endif

/// initial progress bar
extern void progress_init(struct progress_bar *p, int start, int stop, int res, int size)
{
	time_t now;
	time(&now);
        p->start = start;
        p->stop = stop;
        p->unit = 100.0 / (stop - start);
	p->time = now;
	p->block_size = size;
        p->resolution = res;
	p->rate = 0.0;
}

/// update information at progress bar
extern void progress_update(struct progress_bar *p, unsigned long long current, int done, int limit)
{
        setlocale(LC_ALL, "");
        bindtextdomain(PACKAGE, LOCALEDIR);
        textdomain(PACKAGE);
	
	float percent;
        double speedps = 1.0;
        float speed = 1.0;
        int display = 0;
        time_t remained;
	time_t elapsed;
	time_t total;
	char *format = "%H:%M:%S";
	char Rformated[10], Eformated[10], Tformated[10];
	struct tm *Rtm, *Etm, *Ttm;
	char *clear_buf = NULL;
	limit = current % limit;

	if ((limit == 0) || (done == 1)){
	    percent  = p->unit * current;
	    elapsed  = (time(0) - p->time);
	    if (elapsed <= 0)
		elapsed = 1;
	    speedps  = (float)p->block_size * (float)current / (float)(elapsed);
	    //remained = (time_t)(p->block_size * (p->stop- current)/(int)speedps);
	    remained = (time_t)((elapsed/percent*100) - elapsed);
	    speed = (float)(speedps / 1000000.0 * 60.0);
	    p->rate = p->rate+speed;

	    /// format time string
	    Rtm = gmtime(&remained);
	    strftime(Rformated, sizeof(Rformated), format, Rtm);

	    Etm = gmtime(&elapsed);
	    strftime(Eformated, sizeof(Eformated), format, Etm);

	    if (done != 1){
		if (((current - p->start) % p->resolution) && ((current != p->stop)))
		    return;
		fprintf(stderr, _("\r%81c\rElapsed: %s, Remaining: %s, Completed:%6.2f%%, Rate: %6.2fMB/min, "), clear_buf, Eformated, Rformated, percent, (float)(speed));
		/*
		   fprintf(stderr, ("\r%81c\r"), clear_buf);
		   fprintf(stderr, _("Elapsed: %s, "), Eformated);
		   fprintf(stderr, _("Remaining: %s, "), Rformated);
		   fprintf(stderr, _("Completed:%6.2f%%, "), percent);
		   fprintf(stderr, _("Rate:%6.1fMB/min, "), (float)(p->rate));
		   */
	    } else {
		total = elapsed;
		Ttm = gmtime(&total);
		strftime(Tformated, sizeof(Tformated), format, Ttm);
		fprintf(stderr, _("\nTotal Time: %s, "), Tformated);
		fprintf(stderr, _("Ave. Rate: %6.1fMB/min, "), (float)(p->rate/p->stop));
		fprintf(stderr, _("100.00%% completed!\n"));
	    }
	}
}

/// update information at ncurses mode
extern void Ncurses_progress_update(struct progress_bar *p, unsigned long long current, int done, int limit)
{
#ifdef HAVE_LIBNCURSESW

    float percent;
    double speedps = 1.0;
    float speed = 1.0;
    int display = 0;
    time_t remained;
	time_t elapsed;
	time_t total;
	char *format = "%H:%M:%S";
	char Rformated[10], Eformated[10], Tformated[10];
	struct tm *Rtm, *Etm, *Ttm;
	char *clear_buf = NULL;
	char *p_block;
	limit = current % limit;

	if ((limit == 0) || (done == 1)){

		percent  = p->unit * current;
		elapsed  = (time(0) - p->time);
		if (elapsed <= 0)
			elapsed = 1;
		speedps  = (float)p->block_size * (float)current / (float)(elapsed);
		//remained = (time_t)(p->block_size * (p->stop- current)/(int)speedps);
		remained = (time_t)((elapsed/percent*100) - elapsed);
		speed = (float)(speedps / 1000000.0 * 60.0);
		p->rate = p->rate+speed;

		/// format time string
		Rtm = gmtime(&remained);
		strftime(Rformated, sizeof(Rformated), format, Rtm);

		Etm = gmtime(&elapsed);
		strftime(Eformated, sizeof(Eformated), format, Etm);

		/// set bar color
		init_pair(4, COLOR_RED, COLOR_RED);
		init_pair(5, COLOR_WHITE, COLOR_BLUE);
		init_pair(6, COLOR_WHITE, COLOR_RED);
		werase(p_win);
		werase(bar_win);

		if (done != 1){
			if (((current - p->start) % p->resolution) && ((current != p->stop)))
				return;
			mvwprintw(p_win, 0, 0, _(" "));
			mvwprintw(p_win, 1, 0, _("Elapsed: %s") , Eformated);
			mvwprintw(p_win, 2, 0, _("Remaining: %s"), Rformated);
			mvwprintw(p_win, 3, 0, _("Rate: %6.2fMB/min"), (float)(speed));
			//mvwprintw(p_win, 3, 0, _("Completed:%6.2f%%"), percent);
			p_block = malloc(50);
			memset(p_block, 0, 50);
			memset(p_block, ' ', (size_t)(percent*0.5));
			wattrset(bar_win, COLOR_PAIR(4));
			mvwprintw(bar_win, 0, 0, "%s", p_block);
			wattroff(bar_win, COLOR_PAIR(4));
			if(percent <= 50){
				wattrset(p_win, COLOR_PAIR(5));
				mvwprintw(p_win, 5, 25, "%3.0f%%", percent);
				wattroff(p_win, COLOR_PAIR(5));
			}else{
				wattrset(p_win, COLOR_PAIR(6));
				mvwprintw(p_win, 5, 25, "%3.0f%%", percent);
				wattroff(p_win, COLOR_PAIR(6));
			}
			mvwprintw(p_win, 5, 52, "%6.2f%%", percent);
			wrefresh(p_win);
			wrefresh(bar_win);
			free(p_block);
		} else {
			total = elapsed;
			Ttm = gmtime(&total);
			strftime(Tformated, sizeof(Tformated), format, Ttm);
			mvwprintw(p_win, 1, 0, _("Total Time: %s"), Tformated);
			mvwprintw(p_win, 2, 0, _("Remaining: 0"));
			mvwprintw(p_win, 3, 0, _("Ave. Rate: %6.1fMB/min"), (float)(p->rate/p->stop));
			//mvwprintw(p_win, 3, 0, _("100.00%% completed!"));
			wattrset(bar_win, COLOR_PAIR(4));
			mvwprintw(bar_win, 0, 0, "%50s", " ");
			wattroff(bar_win, COLOR_PAIR(4));
			wattrset(p_win, COLOR_PAIR(6));
			mvwprintw(p_win, 5, 22, "%6.2f%%", percent);
			wattroff(p_win, COLOR_PAIR(6));
			mvwprintw(p_win, 5, 52, "%6.2f%%", percent);
			wrefresh(p_win);
			wrefresh(bar_win);
			refresh();
			sleep(1);
		}

		if(done == 1){
			window_f = close_p_ncurses();
		}
	} //end of limit

#endif
}

static int open_p_ncurses(){

	return 1;
}

static int close_p_ncurses(){

	return 1;
}
/// update information as dialog format, refernece source code of dialog
/// # mkfifo pipe
/// # (./clone.extfs -d -c -X -s /dev/loop0 2>pipe | cat - > test.img) | ./gauge < pipe
/// # (cat test - |./clone.extfs -d -X -r -s - -o /dev/loop0 2>pipe) | ./gauge < pipe
extern void Dialog_progress_update(struct progress_bar *p, unsigned long long current, int done, int limit){
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	extern p_dialog_mesg m_dialog;
	
	float percent;
        double speedps = 1.0;
        float speed = 1.0;
        int display = 0;
        time_t remained;
	time_t elapsed;
	time_t total;
	char *format = "%H:%M:%S";
	char Rformated[10], Eformated[10], Tformated[10];
	struct tm *Rtm, *Etm, *Ttm;
	char tmp_str[128];
	char *clear_buf = NULL;
	limit = current % limit;

	if ((limit == 0) || (done == 1)){

		percent  = p->unit * current;
		elapsed  = (time(0) - p->time);
		if (elapsed <= 0)
			elapsed = 1;
		speedps  = (float)p->block_size * (float)current / (float)(elapsed);
		//remained = (time_t)(p->block_size * (p->stop- current)/(int)speedps);
		remained = (time_t)((elapsed/percent*100) - elapsed);
		speed = (float)(speedps / 1000000.0 * 60.0);
		p->rate = p->rate+speed;

		/// format time string
		Rtm = gmtime(&remained);
		strftime(Rformated, sizeof(Rformated), format, Rtm);

		Etm = gmtime(&elapsed);
		strftime(Eformated, sizeof(Eformated), format, Etm);

		if (done != 1){
			if (((current - p->start) % p->resolution) && ((current != p->stop)))
				return;
			m_dialog.percent = (int)percent;
			sprintf(tmp_str, _("  Elapsed: %s\n  Remaining: %s\n  Completed:%6.2f%%\n  Rate: %6.2fMB/min, "), Eformated, Rformated, percent, (float)(speed));
			fprintf(stderr, "XXX\n%i\n%s\n%s\nXXX\n", m_dialog.percent, m_dialog.data, tmp_str);
		} else {
			total = elapsed;
			Ttm = gmtime(&total);
			strftime(Tformated, sizeof(Tformated), format, Ttm);
			m_dialog.percent = 100;
			sprintf(tmp_str, _("  Total Time: %s\n  Ave. Rate: %6.1fMB/min\n 100.00%%  completed!\n"), Tformated, (float)(p->rate/p->stop));
			fprintf(stderr, "XXX\n%i\n%s\n%s\nXXX\n", m_dialog.percent, m_dialog.data, tmp_str);
		}
	}

}