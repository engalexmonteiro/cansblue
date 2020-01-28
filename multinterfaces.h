/*
 * multinterfaces.h
 *
 *  Created on: Aug 7, 2013
 *      Author: alex
 */

#ifndef MULTINTERFACES_H_
#define MULTINTERFACES_H_

#define CANAL 14


#include <NetworkManager.h>

typedef struct wireless_scan_mi
{
  /* Linked list */
  struct wireless_scan_mi *	next;
  struct wireless_scan_mi *	previus;

  struct timeval time_lastsee;
  int			  lastsee;

  /* For All */
  int 		type;					/* Bluetooth or Wi-fi 	*/

  NMDevice 				*device;
  NMActiveConnection 	*active;
  NMConnection 			*connection;
  NMSettingConnection 	*s_con;

  char		ifname[10];
  char      ip_address[50];   		/* IP Address 			*/
  char		mac_address[50];		/* Access point address */
  char		net_mask[50];			/* Access point address */
  char		gw_address[50];			/* Access point address */
  char 		dns1_address[50];
  char 		dns2_address[50];
  char		protocol[50];			/* Access point address */
  int 		connected;				/*connectado 			*/


  int 		cont;
  int 		current_bw;				/* Current Bandwith 	*/

  float		qualidade;				/* Quality for wi-fi 	*/
  float 	qual_sum;
  int 		qual_cont;
  float 	qual_avg;

  int		nivel;					/* Level for bluetooth or wi-fi */
  float 	sum_level;
  int 		cont_level;
  float		avg_level;

  int 		service;					/*Service disponibility */
  float 	sum_service;				/*Service disponibility */
  int	 	cont_service;				/*Service disponibility */
  float	 	avg_service;				/*Service disponibility */

  float 	sum_score;
  int 	    cont_score;
  float 	avg_score;
  float 	score;

  char		essid[50];				/* Access point address */

  //for Bluetooth
  int 		afh;
  int       map_afh_int[80];         /* AFH Map for bluetooth */
  int 		good_channels;
  int 		bad_channels;

  //for Wifi
  int 		canal;					/* Channel for wi-fi 	*/
  int		key;
  float 	frequencia;				/* Frequence for wi-fi */
  int 		maxbitrate;				/* Maxbitrate for wi-fi */
  char 		modo[50];				/* Mode for blueooth: slave or master and wi-fi */

  float 	factor_overlap;			/* Factor of Overlap */
  float 	sum_olf;
  int 		cont_olf;
  float 	avg_olf;

  int		score_snr;

} wireless_scan_mi;


/*
 * Context used for non-blocking scan.
 */
typedef struct wireless_scan_mi_list
{
  struct timeval 		time;
  wireless_scan_mi*		head_list;				/* Result of the scan */
  wireless_scan_mi*		end_list;
  wireless_scan_mi*		best_ap;				/* The best of AP */
  int 					channel_util[CANAL];	/* Canais utilizados 802.11 */
  float 				fator_sobreposicao[CANAL];	/* Canais utilizados 802.11 */
  float					max_olf;
  int					size_list;		        /* Tamanho da lista */
  int 					factor_diversity;
  int				 	melhores_canais[CANAL];

} wireless_scan_mi_list;


/* Modes as human readable strings */
static char *wi_type_device[2] = {"802.11","802.15.1"};

#endif /* MULTINTERFACES_H_ */
