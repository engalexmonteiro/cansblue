/*
 * mod802151.h
 *
 *  Created on: Aug 7, 2013
 *      Author: alex
 */

#ifndef MOD802151_H_
#define MOD802151_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <errno.h>

#include "multinterfaces.h"


int convertebinario(uint8_t *map, int* map_afh_int);

int get_myblue_info(wireless_scan_mi* bluenet);

int connect_scan(int sock, int dev_id);

int inquiry_scan(int sock, int dev_id);

int scan_bluetooth_service(wireless_scan_mi_list *blueap, char *serv);

int iw_scan_bluetooth(wireless_scan_mi_list* list);

#endif /* MOD802151_H_ */
