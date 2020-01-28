/*
 * mod802151.c
 *
 *  Created on: Aug 7, 2013
 *      Author: alex
 */


#include "cansblue.h"

static char UUID_str[MAX_LEN_UUID_STR];

/* Pass args to the inquiry/search handler */
struct search_context {
	char		*svc;		/* Service */
	uuid_t		group;		/* Browse group */
	int		view;		/* View mode */
	uint32_t	handle;		/* Service record handle */
};

static bdaddr_t interface;

typedef int (*handler_t)(bdaddr_t *bdaddr, struct search_context *arg);

typedef struct {
	uint32_t handle;
	char *name;
	char *provider;
	char *desc;
	unsigned int class;
	unsigned int profile;
	uint16_t psm;
	uint8_t channel;
	uint8_t network;
} svc_info_t;

static void add_lang_attr(sdp_record_t *r)
{
	sdp_lang_attr_t base_lang;
	sdp_list_t *langs = 0;

	/* UTF-8 MIBenum (http://www.iana.org/assignments/character-sets) */
	base_lang.code_ISO639 = (0x65 << 8) | 0x6e;
	base_lang.encoding = 106;
	base_lang.base_offset = SDP_PRIMARY_LANG_BASE;
	langs = sdp_list_append(0, &base_lang);
	sdp_set_lang_attr(r, langs);
	sdp_list_free(langs, 0);
}

static int add_sp(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *apseq, *proto[2], *profiles, *root, *aproto;
	uuid_t root_uuid, sp_uuid, l2cap, rfcomm;
	sdp_profile_desc_t profile;
	sdp_record_t record;
	uint8_t u8 = si->channel ? si->channel : 1;
	sdp_data_t *channel;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);
	sdp_list_free(root, 0);

	sdp_uuid16_create(&sp_uuid, SERIAL_PORT_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &sp_uuid);
	sdp_set_service_classes(&record, svclass_id);
	sdp_list_free(svclass_id, 0);

	sdp_uuid16_create(&profile.uuid, SERIAL_PORT_PROFILE_ID);
	profile.version = 0x0100;
	profiles = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, profiles);
	sdp_list_free(profiles, 0);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	add_lang_attr(&record);

	sdp_set_info_attr(&record, "Serial Port", "BlueZ", "COM Port");

	sdp_set_url_attr(&record, "http://www.bluez.org/",
			"http://www.bluez.org/", "http://www.bluez.org/");

	sdp_set_service_id(&record, sp_uuid);
	sdp_set_service_ttl(&record, 0xffff);
	sdp_set_service_avail(&record, 0xff);
	sdp_set_record_state(&record, 0x00001234);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("Serial Port service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_dun(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root, *aproto;
	uuid_t rootu, dun, gn, l2cap, rfcomm;
	sdp_profile_desc_t profile;
	sdp_list_t *proto[2];
	sdp_record_t record;
	uint8_t u8 = si->channel ? si->channel : 2;
	sdp_data_t *channel;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&rootu, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &rootu);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&dun, DIALUP_NET_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &dun);
	sdp_uuid16_create(&gn,  GENERIC_NETWORKING_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &gn);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile.uuid, DIALUP_NET_PROFILE_ID);
	profile.version = 0x0100;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Dial-Up Networking", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("Dial-Up Networking service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_fax(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, fax_uuid, tel_uuid, l2cap_uuid, rfcomm_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint8_t u8 = si->channel? si->channel : 3;
	sdp_data_t *channel;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&fax_uuid, FAX_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &fax_uuid);
	sdp_uuid16_create(&tel_uuid, GENERIC_TELEPHONY_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &tel_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile.uuid, FAX_PROFILE_ID);
	profile.version = 0x0100;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq  = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Fax", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}
	printf("Fax service registered\n");
end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);
	return ret;
}

static int add_lan(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, svclass_uuid, l2cap_uuid, rfcomm_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint8_t u8 = si->channel ? si->channel : 4;
	sdp_data_t *channel;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&svclass_uuid, LAN_ACCESS_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &svclass_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile.uuid, LAN_ACCESS_PROFILE_ID);
	profile.version = 0x0100;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "LAN Access over PPP", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("LAN Access service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_headset(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, svclass_uuid, ga_svclass_uuid, l2cap_uuid, rfcomm_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint8_t u8 = si->channel ? si->channel : 5;
	sdp_data_t *channel;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&svclass_uuid, HEADSET_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &svclass_uuid);
	sdp_uuid16_create(&ga_svclass_uuid, GENERIC_AUDIO_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &ga_svclass_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile.uuid, HEADSET_PROFILE_ID);
	profile.version = 0x0100;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Headset", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("Headset service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_headset_ag(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, svclass_uuid, ga_svclass_uuid, l2cap_uuid, rfcomm_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint8_t u8 = si->channel ? si->channel : 7;
	sdp_data_t *channel;
	uint8_t netid = si->network ? si->network : 0x01; // ???? profile document
	sdp_data_t *network = sdp_data_alloc(SDP_UINT8, &netid);
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&svclass_uuid, HEADSET_AGW_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &svclass_uuid);
	sdp_uuid16_create(&ga_svclass_uuid, GENERIC_AUDIO_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &ga_svclass_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile.uuid, HEADSET_PROFILE_ID);
	profile.version = 0x0100;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Voice Gateway", 0, 0);

	sdp_attr_add(&record, SDP_ATTR_EXTERNAL_NETWORK, network);

	if (sdp_record_register(session, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("Headset AG service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_handsfree(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, svclass_uuid, ga_svclass_uuid, l2cap_uuid, rfcomm_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint8_t u8 = si->channel ? si->channel : 6;
	uint16_t u16 = 0x31;
	sdp_data_t *channel, *features;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&svclass_uuid, HANDSFREE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &svclass_uuid);
	sdp_uuid16_create(&ga_svclass_uuid, GENERIC_AUDIO_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &ga_svclass_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile.uuid, HANDSFREE_PROFILE_ID);
	profile.version = 0x0101;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	features = sdp_data_alloc(SDP_UINT16, &u16);
	sdp_attr_add(&record, SDP_ATTR_SUPPORTED_FEATURES, features);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Handsfree", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("Handsfree service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_handsfree_ag(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, svclass_uuid, ga_svclass_uuid, l2cap_uuid, rfcomm_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint8_t u8 = si->channel ? si->channel : 7;
	uint16_t u16 = 0x17;
	sdp_data_t *channel, *features;
	uint8_t netid = si->network ? si->network : 0x01; // ???? profile document
	sdp_data_t *network = sdp_data_alloc(SDP_UINT8, &netid);
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&svclass_uuid, HANDSFREE_AGW_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &svclass_uuid);
	sdp_uuid16_create(&ga_svclass_uuid, GENERIC_AUDIO_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &ga_svclass_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile.uuid, HANDSFREE_PROFILE_ID);
	profile.version = 0x0105;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	features = sdp_data_alloc(SDP_UINT16, &u16);
	sdp_attr_add(&record, SDP_ATTR_SUPPORTED_FEATURES, features);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Voice Gateway", 0, 0);

	sdp_attr_add(&record, SDP_ATTR_EXTERNAL_NETWORK, network);

	if (sdp_record_register(session, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("Handsfree AG service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_simaccess(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, svclass_uuid, ga_svclass_uuid, l2cap_uuid, rfcomm_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint8_t u8 = si->channel? si->channel : 8;
	uint16_t u16 = 0x31;
	sdp_data_t *channel, *features;
	int ret = 0;

	memset((void *)&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&svclass_uuid, SAP_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &svclass_uuid);
	sdp_uuid16_create(&ga_svclass_uuid, GENERIC_TELEPHONY_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &ga_svclass_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile.uuid, SAP_PROFILE_ID);
	profile.version = 0x0101;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	features = sdp_data_alloc(SDP_UINT16, &u16);
	sdp_attr_add(&record, SDP_ATTR_SUPPORTED_FEATURES, features);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "SIM Access", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("SIM Access service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_opush(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, opush_uuid, l2cap_uuid, rfcomm_uuid, obex_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	sdp_record_t record;
	uint8_t chan = si->channel ? si->channel : 9;
	sdp_data_t *channel;
	uint8_t formats[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0xff };
	void *dtds[sizeof(formats)], *values[sizeof(formats)];
	unsigned int i;
	uint8_t dtd = SDP_UINT8;
	sdp_data_t *sflist;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&opush_uuid, OBEX_OBJPUSH_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &opush_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, OBEX_OBJPUSH_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &chan);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	sdp_uuid16_create(&obex_uuid, OBEX_UUID);
	proto[2] = sdp_list_append(0, &obex_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	for (i = 0; i < sizeof(formats); i++) {
		dtds[i] = &dtd;
		values[i] = &formats[i];
	}
	sflist = sdp_seq_alloc(dtds, values, sizeof(formats));
	sdp_attr_add(&record, SDP_ATTR_SUPPORTED_FORMATS_LIST, sflist);

	sdp_set_info_attr(&record, "OBEX Object Push", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("OBEX Object Push service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(proto[2], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_pbap(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, pbap_uuid, l2cap_uuid, rfcomm_uuid, obex_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	sdp_record_t record;
	uint8_t chan = si->channel ? si->channel : 19;
	sdp_data_t *channel;
	uint8_t formats[] = {0x01};
	uint8_t dtd = SDP_UINT8;
	sdp_data_t *sflist;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&pbap_uuid, PBAP_PSE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &pbap_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, PBAP_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &chan);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	sdp_uuid16_create(&obex_uuid, OBEX_UUID);
	proto[2] = sdp_list_append(0, &obex_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sflist = sdp_data_alloc(dtd,formats);
	sdp_attr_add(&record, SDP_ATTR_SUPPORTED_REPOSITORIES, sflist);

	sdp_set_info_attr(&record, "OBEX Phonebook Access Server", 0, 0);

	if (sdp_device_record_register(session, &interface, &record,
			SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("PBAP service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(proto[2], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_ftp(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, ftrn_uuid, l2cap_uuid, rfcomm_uuid, obex_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	sdp_record_t record;
	uint8_t u8 = si->channel ? si->channel: 10;
	sdp_data_t *channel;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&ftrn_uuid, OBEX_FILETRANS_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &ftrn_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, OBEX_FILETRANS_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	sdp_uuid16_create(&obex_uuid, OBEX_UUID);
	proto[2] = sdp_list_append(0, &obex_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "OBEX File Transfer", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("OBEX File Transfer service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(proto[2], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_directprint(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, opush_uuid, l2cap_uuid, rfcomm_uuid, obex_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	sdp_record_t record;
	uint8_t chan = si->channel ? si->channel : 12;
	sdp_data_t *channel;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&opush_uuid, DIRECT_PRINTING_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &opush_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, BASIC_PRINTING_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &chan);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	sdp_uuid16_create(&obex_uuid, OBEX_UUID);
	proto[2] = sdp_list_append(0, &obex_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Direct Printing", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("Direct Printing service registered\n");

end:
	sdp_data_free(channel);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(proto[2], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_nap(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, ftrn_uuid, l2cap_uuid, bnep_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint16_t lp = 0x000f, ver = 0x0100;
	sdp_data_t *psm, *version;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&ftrn_uuid, NAP_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &ftrn_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, NAP_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&bnep_uuid, BNEP_UUID);
	proto[1] = sdp_list_append(0, &bnep_uuid);
	version  = sdp_data_alloc(SDP_UINT16, &ver);
	proto[1] = sdp_list_append(proto[1], version);

	{
		uint16_t ptype[4] = { 0x0010, 0x0020, 0x0030, 0x0040 };
		sdp_data_t *head, *pseq;
		int p;

		for (p = 0, head = NULL; p < 4; p++) {
			sdp_data_t *data = sdp_data_alloc(SDP_UINT16, &ptype[p]);
			head = sdp_seq_append(head, data);
		}
		pseq = sdp_data_alloc(SDP_SEQ16, head);
		proto[1] = sdp_list_append(proto[1], pseq);
	}

	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Network Access Point Service", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("NAP service registered\n");

end:
	sdp_data_free(version);
	sdp_data_free(psm);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_gn(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, ftrn_uuid, l2cap_uuid, bnep_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint16_t lp = 0x000f, ver = 0x0100;
	sdp_data_t *psm, *version;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&ftrn_uuid, GN_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &ftrn_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, GN_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&bnep_uuid, BNEP_UUID);
	proto[1] = sdp_list_append(0, &bnep_uuid);
	version = sdp_data_alloc(SDP_UINT16, &ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Group Network Service", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("GN service registered\n");

end:
	sdp_data_free(version);
	sdp_data_free(psm);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_panu(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, ftrn_uuid, l2cap_uuid, bnep_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint16_t lp = 0x000f, ver = 0x0100;
	sdp_data_t *psm, *version;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);
	sdp_list_free(root, NULL);

	sdp_uuid16_create(&ftrn_uuid, PANU_SVCLASS_ID);
	svclass_id = sdp_list_append(NULL, &ftrn_uuid);
	sdp_set_service_classes(&record, svclass_id);
	sdp_list_free(svclass_id, NULL);

	sdp_uuid16_create(&profile[0].uuid, PANU_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(NULL, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);
	sdp_list_free(pfseq, NULL);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(NULL, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(NULL, proto[0]);

	sdp_uuid16_create(&bnep_uuid, BNEP_UUID);
	proto[1] = sdp_list_append(NULL, &bnep_uuid);
	version = sdp_data_alloc(SDP_UINT16, &ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(NULL, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "PAN User", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("PANU service registered\n");

end:
	sdp_data_free(version);
	sdp_data_free(psm);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_hid_keyb(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, hidkb_uuid, l2cap_uuid, hidp_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	sdp_data_t *psm, *lang_lst, *lang_lst2, *hid_spec_lst, *hid_spec_lst2;
	unsigned int i;
	uint8_t dtd = SDP_UINT16;
	uint8_t dtd2 = SDP_UINT8;
	uint8_t dtd_data = SDP_TEXT_STR8;
	void *dtds[2];
	void *values[2];
	void *dtds2[2];
	void *values2[2];
	int leng[2];
	uint8_t hid_spec_type = 0x22;
	uint16_t hid_attr_lang[] = { 0x409, 0x100 };
	static const uint16_t ctrl = 0x11;
	static const uint16_t intr = 0x13;
	static const uint16_t hid_attr[] = { 0x100, 0x111, 0x40, 0x0d, 0x01, 0x01 };
	static const uint16_t hid_attr2[] = { 0x0, 0x01, 0x100, 0x1f40, 0x01, 0x01 };
	const uint8_t hid_spec[] = {
		0x05, 0x01, // usage page
		0x09, 0x06, // keyboard
		0xa1, 0x01, // key codes
		0x85, 0x01, // minimum
		0x05, 0x07, // max
		0x19, 0xe0, // logical min
		0x29, 0xe7, // logical max
		0x15, 0x00, // report size
		0x25, 0x01, // report count
		0x75, 0x01, // input data variable absolute
		0x95, 0x08, // report count
		0x81, 0x02, // report size
		0x75, 0x08,
		0x95, 0x01,
		0x81, 0x01,
		0x75, 0x01,
		0x95, 0x05,
		0x05, 0x08,
		0x19, 0x01,
		0x29, 0x05,
		0x91, 0x02,
		0x75, 0x03,
		0x95, 0x01,
		0x91, 0x01,
		0x75, 0x08,
		0x95, 0x06,
		0x15, 0x00,
		0x26, 0xff,
		0x00, 0x05,
		0x07, 0x19,
		0x00, 0x2a,
		0xff, 0x00,
		0x81, 0x00,
		0x75, 0x01,
		0x95, 0x01,
		0x15, 0x00,
		0x25, 0x01,
		0x05, 0x0c,
		0x09, 0xb8,
		0x81, 0x06,
		0x09, 0xe2,
		0x81, 0x06,
		0x09, 0xe9,
		0x81, 0x02,
		0x09, 0xea,
		0x81, 0x02,
		0x75, 0x01,
		0x95, 0x04,
		0x81, 0x01,
		0xc0         // end tag
	};

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	add_lang_attr(&record);

	sdp_uuid16_create(&hidkb_uuid, HID_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &hidkb_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, HID_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, profile);
	sdp_set_profile_descs(&record, pfseq);

	/* protocols */
	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[1] = sdp_list_append(0, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &ctrl);
	proto[1] = sdp_list_append(proto[1], psm);
	apseq = sdp_list_append(0, proto[1]);

	sdp_uuid16_create(&hidp_uuid, HIDP_UUID);
	proto[2] = sdp_list_append(0, &hidp_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	/* additional protocols */
	proto[1] = sdp_list_append(0, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &intr);
	proto[1] = sdp_list_append(proto[1], psm);
	apseq = sdp_list_append(0, proto[1]);

	sdp_uuid16_create(&hidp_uuid, HIDP_UUID);
	proto[2] = sdp_list_append(0, &hidp_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_add_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "HID Keyboard", NULL, NULL);

	for (i = 0; i < sizeof(hid_attr) / 2; i++)
		sdp_attr_add_new(&record,
					SDP_ATTR_HID_DEVICE_RELEASE_NUMBER + i,
					SDP_UINT16, &hid_attr[i]);

	dtds[0] = &dtd2;
	values[0] = &hid_spec_type;
	dtds[1] = &dtd_data;
	values[1] = (uint8_t *) hid_spec;
	leng[0] = 0;
	leng[1] = sizeof(hid_spec);
	hid_spec_lst = sdp_seq_alloc_with_length(dtds, values, leng, 2);
	hid_spec_lst2 = sdp_data_alloc(SDP_SEQ8, hid_spec_lst);
	sdp_attr_add(&record, SDP_ATTR_HID_DESCRIPTOR_LIST, hid_spec_lst2);

	for (i = 0; i < sizeof(hid_attr_lang) / 2; i++) {
		dtds2[i] = &dtd;
		values2[i] = &hid_attr_lang[i];
	}

	lang_lst = sdp_seq_alloc(dtds2, values2, sizeof(hid_attr_lang) / 2);
	lang_lst2 = sdp_data_alloc(SDP_SEQ8, lang_lst);
	sdp_attr_add(&record, SDP_ATTR_HID_LANG_ID_BASE_LIST, lang_lst2);

	sdp_attr_add_new(&record, SDP_ATTR_HID_SDP_DISABLE, SDP_UINT16, &hid_attr2[0]);

	for (i = 0; i < sizeof(hid_attr2) / 2 - 1; i++)
		sdp_attr_add_new(&record, SDP_ATTR_HID_REMOTE_WAKEUP + i,
						SDP_UINT16, &hid_attr2[i + 1]);

	if (sdp_record_register(session, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("HID keyboard service registered\n");

	return 0;
}

static int add_hid_wiimote(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, hid_uuid, l2cap_uuid, hidp_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	sdp_data_t *psm, *lang_lst, *lang_lst2, *hid_spec_lst, *hid_spec_lst2;
	unsigned int i;
	uint8_t dtd = SDP_UINT16;
	uint8_t dtd2 = SDP_UINT8;
	uint8_t dtd_data = SDP_TEXT_STR8;
	void *dtds[2];
	void *values[2];
	void *dtds2[2];
	void *values2[2];
	int leng[2];
	uint8_t hid_spec_type = 0x22;
	uint16_t hid_attr_lang[] = { 0x409, 0x100 };
	uint16_t ctrl = 0x11, intr = 0x13;
	uint16_t hid_release = 0x0100, parser_version = 0x0111;
	uint8_t subclass = 0x04, country = 0x33;
	uint8_t virtual_cable = 0, reconnect = 1, sdp_disable = 0;
	uint8_t battery = 1, remote_wakeup = 1;
	uint16_t profile_version = 0x0100, superv_timeout = 0x0c80;
	uint8_t norm_connect = 0, boot_device = 0;
	const uint8_t hid_spec[] = {
		0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x10,
		0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95,
		0x01, 0x06, 0x00, 0xff, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x11, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x12, 0x95, 0x02, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x13, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x14, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x15, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x16, 0x95, 0x15, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x17, 0x95, 0x06, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x18, 0x95, 0x15, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x19, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x1a, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x20, 0x95, 0x06, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x21, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x22, 0x95, 0x04, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x30, 0x95, 0x02, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x31, 0x95, 0x05, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x32, 0x95, 0x0a, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x33, 0x95, 0x11, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x34, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x35, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x36, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x37, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x3d, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x3e, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x3f, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0xc0, 0x00
	};

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&hid_uuid, HID_SVCLASS_ID);
	svclass_id = sdp_list_append(NULL, &hid_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, HID_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(NULL, profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[1] = sdp_list_append(0, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &ctrl);
	proto[1] = sdp_list_append(proto[1], psm);
	apseq = sdp_list_append(0, proto[1]);

	sdp_uuid16_create(&hidp_uuid, HIDP_UUID);
	proto[2] = sdp_list_append(0, &hidp_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	proto[1] = sdp_list_append(0, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &intr);
	proto[1] = sdp_list_append(proto[1], psm);
	apseq = sdp_list_append(0, proto[1]);

	sdp_uuid16_create(&hidp_uuid, HIDP_UUID);
	proto[2] = sdp_list_append(0, &hidp_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_add_access_protos(&record, aproto);

	add_lang_attr(&record);

	sdp_set_info_attr(&record, "Nintendo RVL-CNT-01",
					"Nintendo", "Nintendo RVL-CNT-01");

	sdp_attr_add_new(&record, SDP_ATTR_HID_DEVICE_RELEASE_NUMBER,
						SDP_UINT16, &hid_release);

	sdp_attr_add_new(&record, SDP_ATTR_HID_PARSER_VERSION,
						SDP_UINT16, &parser_version);

	sdp_attr_add_new(&record, SDP_ATTR_HID_DEVICE_SUBCLASS,
						SDP_UINT8, &subclass);

	sdp_attr_add_new(&record, SDP_ATTR_HID_COUNTRY_CODE,
						SDP_UINT8, &country);

	sdp_attr_add_new(&record, SDP_ATTR_HID_VIRTUAL_CABLE,
						SDP_BOOL, &virtual_cable);

	sdp_attr_add_new(&record, SDP_ATTR_HID_RECONNECT_INITIATE,
						SDP_BOOL, &reconnect);

	dtds[0] = &dtd2;
	values[0] = &hid_spec_type;
	dtds[1] = &dtd_data;
	values[1] = (uint8_t *) hid_spec;
	leng[0] = 0;
	leng[1] = sizeof(hid_spec);
	hid_spec_lst = sdp_seq_alloc_with_length(dtds, values, leng, 2);
	hid_spec_lst2 = sdp_data_alloc(SDP_SEQ8, hid_spec_lst);
	sdp_attr_add(&record, SDP_ATTR_HID_DESCRIPTOR_LIST, hid_spec_lst2);

	for (i = 0; i < sizeof(hid_attr_lang) / 2; i++) {
		dtds2[i] = &dtd;
		values2[i] = &hid_attr_lang[i];
	}

	lang_lst = sdp_seq_alloc(dtds2, values2, sizeof(hid_attr_lang) / 2);
	lang_lst2 = sdp_data_alloc(SDP_SEQ8, lang_lst);
	sdp_attr_add(&record, SDP_ATTR_HID_LANG_ID_BASE_LIST, lang_lst2);

	sdp_attr_add_new(&record, SDP_ATTR_HID_SDP_DISABLE,
						SDP_BOOL, &sdp_disable);

	sdp_attr_add_new(&record, SDP_ATTR_HID_BATTERY_POWER,
						SDP_BOOL, &battery);

	sdp_attr_add_new(&record, SDP_ATTR_HID_REMOTE_WAKEUP,
						SDP_BOOL, &remote_wakeup);

	sdp_attr_add_new(&record, SDP_ATTR_HID_PROFILE_VERSION,
						SDP_UINT16, &profile_version);

	sdp_attr_add_new(&record, SDP_ATTR_HID_SUPERVISION_TIMEOUT,
						SDP_UINT16, &superv_timeout);

	sdp_attr_add_new(&record, SDP_ATTR_HID_NORMALLY_CONNECTABLE,
						SDP_BOOL, &norm_connect);

	sdp_attr_add_new(&record, SDP_ATTR_HID_BOOT_DEVICE,
						SDP_BOOL, &boot_device);

	if (sdp_record_register(session, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("Wii-Mote service registered\n");

	return 0;
}

static int add_cip(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, cmtp, cip;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint16_t psm = si->psm ? si->psm : 0x1001;
	uint8_t netid = si->network ? si->network : 0x02; // 0x02 = ISDN, 0x03 = GSM
	sdp_data_t *network = sdp_data_alloc(SDP_UINT8, &netid);
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&cip, CIP_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &cip);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, CIP_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	apseq = sdp_list_append(0, proto[0]);
	proto[0] = sdp_list_append(proto[0], sdp_data_alloc(SDP_UINT16, &psm));
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&cmtp, CMTP_UUID);
	proto[1] = sdp_list_append(0, &cmtp);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_attr_add(&record, SDP_ATTR_EXTERNAL_NETWORK, network);

	sdp_set_info_attr(&record, "Common ISDN Access", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("CIP service registered\n");

end:
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);
	sdp_data_free(network);

	return ret;
}

static int add_ctp(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, tcsbin, ctp;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	uint8_t netid = si->network ? si->network : 0x02; // 0x01-0x07 cf. p120 profile document
	sdp_data_t *network = sdp_data_alloc(SDP_UINT8, &netid);
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&ctp, CORDLESS_TELEPHONY_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &ctp);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, CORDLESS_TELEPHONY_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&tcsbin, TCS_BIN_UUID);
	proto[1] = sdp_list_append(0, &tcsbin);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_attr_add(&record, SDP_ATTR_EXTERNAL_NETWORK, network);

	sdp_set_info_attr(&record, "Cordless Telephony", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto end;
	}

	printf("CTP service registered\n");

end:
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);
	sdp_data_free(network);

	return ret;
}

static int add_a2source(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, avdtp, a2src;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	sdp_data_t *psm, *version;
	uint16_t lp = 0x0019, ver = 0x0100;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&a2src, AUDIO_SOURCE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &a2src);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, ADVANCED_AUDIO_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&avdtp, AVDTP_UUID);
	proto[1] = sdp_list_append(0, &avdtp);
	version = sdp_data_alloc(SDP_UINT16, &ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Audio Source", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto done;
	}

	printf("Audio source service registered\n");

done:
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_a2sink(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, avdtp, a2snk;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	sdp_data_t *psm, *version;
	uint16_t lp = 0x0019, ver = 0x0100;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&a2snk, AUDIO_SINK_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &a2snk);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, ADVANCED_AUDIO_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&avdtp, AVDTP_UUID);
	proto[1] = sdp_list_append(0, &avdtp);
	version = sdp_data_alloc(SDP_UINT16, &ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Audio Sink", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto done;
	}

	printf("Audio sink service registered\n");

done:
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_avrct(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, avctp, avrct;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	sdp_data_t *psm, *version, *features;
	uint16_t lp = 0x0017, ver = 0x0100, feat = 0x000f;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&avrct, AV_REMOTE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &avrct);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, AV_REMOTE_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&avctp, AVCTP_UUID);
	proto[1] = sdp_list_append(0, &avctp);
	version = sdp_data_alloc(SDP_UINT16, &ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	features = sdp_data_alloc(SDP_UINT16, &feat);
	sdp_attr_add(&record, SDP_ATTR_SUPPORTED_FEATURES, features);

	sdp_set_info_attr(&record, "AVRCP CT", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto done;
	}

	printf("Remote control service registered\n");

done:
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_avrtg(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, avctp, avrtg;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t record;
	sdp_data_t *psm, *version, *features;
	uint16_t lp = 0x0017, ver = 0x0100, feat = 0x000f;
	int ret = 0;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&avrtg, AV_REMOTE_TARGET_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &avrtg);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, AV_REMOTE_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&avctp, AVCTP_UUID);
	proto[1] = sdp_list_append(0, &avctp);
	version = sdp_data_alloc(SDP_UINT16, &ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	features = sdp_data_alloc(SDP_UINT16, &feat);
	sdp_attr_add(&record, SDP_ATTR_SUPPORTED_FEATURES, features);

	sdp_set_info_attr(&record, "AVRCP TG", 0, 0);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		ret = -1;
		goto done;
	}

	printf("Remote target service registered\n");

done:
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);

	return ret;
}

static int add_udi_ue(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass, *proto;
	uuid_t root_uuid, svclass_uuid, l2cap_uuid, rfcomm_uuid;
	uint8_t channel = si->channel ? si->channel: 18;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);
	sdp_list_free(root, NULL);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(NULL, &l2cap_uuid));

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto = sdp_list_append(proto, sdp_list_append(
		sdp_list_append(NULL, &rfcomm_uuid), sdp_data_alloc(SDP_UINT8, &channel)));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_uuid16_create(&svclass_uuid, UDI_MT_SVCLASS_ID);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);
	sdp_list_free(svclass, NULL);

	sdp_set_info_attr(&record, "UDI UE", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("UDI UE service registered\n");

	return 0;
}

static int add_udi_te(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass, *proto;
	uuid_t root_uuid, svclass_uuid, l2cap_uuid, rfcomm_uuid;
	uint8_t channel = si->channel ? si->channel: 19;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);
	sdp_list_free(root, NULL);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(NULL, &l2cap_uuid));

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto = sdp_list_append(proto, sdp_list_append(
		sdp_list_append(NULL, &rfcomm_uuid), sdp_data_alloc(SDP_UINT8, &channel)));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_uuid16_create(&svclass_uuid, UDI_TA_SVCLASS_ID);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);
	sdp_list_free(svclass, NULL);

	sdp_set_info_attr(&record, "UDI TE", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("UDI TE service registered\n");

	return 0;
}

static unsigned char sr1_uuid[] = {	0xbc, 0x19, 0x9c, 0x24, 0x95, 0x8b, 0x4c, 0xc0,
					0xa2, 0xcb, 0xfd, 0x8a, 0x30, 0xbf, 0x32, 0x06 };

static int add_sr1(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass;
	uuid_t root_uuid, svclass_uuid;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid128_create(&svclass_uuid, (void *) sr1_uuid);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);

	sdp_set_info_attr(&record, "TOSHIBA SR-1", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("Toshiba Speech Recognition SR-1 service record registered\n");

	return 0;
}

static unsigned char syncmls_uuid[] = {	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x02 };

static unsigned char syncmlc_uuid[] = {	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x02 };

static int add_syncml(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass, *proto;
	uuid_t root_uuid, svclass_uuid, l2cap_uuid, rfcomm_uuid, obex_uuid;
	uint8_t channel = si->channel ? si->channel: 15;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid128_create(&svclass_uuid, (void *) syncmlc_uuid);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(NULL, &l2cap_uuid));

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto = sdp_list_append(proto, sdp_list_append(
		sdp_list_append(NULL, &rfcomm_uuid), sdp_data_alloc(SDP_UINT8, &channel)));

	sdp_uuid16_create(&obex_uuid, OBEX_UUID);
	proto = sdp_list_append(proto, sdp_list_append(NULL, &obex_uuid));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_set_info_attr(&record, "SyncML Client", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("SyncML Client service record registered\n");

	return 0;
}

static unsigned char async_uuid[] = {	0x03, 0x50, 0x27, 0x8F, 0x3D, 0xCA, 0x4E, 0x62,
					0x83, 0x1D, 0xA4, 0x11, 0x65, 0xFF, 0x90, 0x6C };

static int add_activesync(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass, *proto;
	uuid_t root_uuid, svclass_uuid, l2cap_uuid, rfcomm_uuid;
	uint8_t channel = si->channel ? si->channel: 21;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(NULL, &l2cap_uuid));

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto = sdp_list_append(proto, sdp_list_append(
	sdp_list_append(NULL, &rfcomm_uuid), sdp_data_alloc(SDP_UINT8, &channel)));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_uuid128_create(&svclass_uuid, (void *) async_uuid);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);

	sdp_set_info_attr(&record, "Microsoft ActiveSync", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("ActiveSync service record registered\n");

	return 0;
}

static unsigned char hotsync_uuid[] = {	0xD8, 0x0C, 0xF9, 0xEA, 0x13, 0x4C, 0x11, 0xD5,
					0x83, 0xCE, 0x00, 0x30, 0x65, 0x7C, 0x54, 0x3C };

static int add_hotsync(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass, *proto;
	uuid_t root_uuid, svclass_uuid, l2cap_uuid, rfcomm_uuid;
	uint8_t channel = si->channel ? si->channel: 22;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(NULL, &l2cap_uuid));

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto = sdp_list_append(proto, sdp_list_append(
	sdp_list_append(NULL, &rfcomm_uuid), sdp_data_alloc(SDP_UINT8, &channel)));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_uuid128_create(&svclass_uuid, (void *) hotsync_uuid);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);

	sdp_set_info_attr(&record, "PalmOS HotSync", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("HotSync service record registered\n");

	return 0;
}

static unsigned char palmos_uuid[] = {	0xF5, 0xBE, 0xB6, 0x51, 0x41, 0x71, 0x40, 0x51,
					0xAC, 0xF5, 0x6C, 0xA7, 0x20, 0x22, 0x42, 0xF0 };

static int add_palmos(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass;
	uuid_t root_uuid, svclass_uuid;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid128_create(&svclass_uuid, (void *) palmos_uuid);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("PalmOS service record registered\n");

	return 0;
}

static unsigned char nokid_uuid[] = {	0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x01 };

static int add_nokiaid(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass;
	uuid_t root_uuid, svclass_uuid;
	uint16_t verid = 0x005f;
	sdp_data_t *version = sdp_data_alloc(SDP_UINT16, &verid);

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid128_create(&svclass_uuid, (void *) nokid_uuid);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);

	sdp_attr_add(&record, SDP_ATTR_SERVICE_VERSION, version);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		sdp_data_free(version);
		return -1;
	}

	printf("Nokia ID service record registered\n");

	return 0;
}

static unsigned char pcsuite_uuid[] = {	0x00, 0x00, 0x50, 0x02, 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x01 };

static int add_pcsuite(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass, *proto;
	uuid_t root_uuid, svclass_uuid, l2cap_uuid, rfcomm_uuid;
	uint8_t channel = si->channel ? si->channel: 14;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(NULL, &l2cap_uuid));

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto = sdp_list_append(proto, sdp_list_append(
		sdp_list_append(NULL, &rfcomm_uuid), sdp_data_alloc(SDP_UINT8, &channel)));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_uuid128_create(&svclass_uuid, (void *) pcsuite_uuid);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);

	sdp_set_info_attr(&record, "Nokia PC Suite", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("Nokia PC Suite service registered\n");

	return 0;
}

static unsigned char nftp_uuid[] = {	0x00, 0x00, 0x50, 0x05, 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x01 };

static unsigned char nsyncml_uuid[] = {	0x00, 0x00, 0x56, 0x01, 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x01 };

static unsigned char ngage_uuid[] = {	0x00, 0x00, 0x13, 0x01, 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x01 };

static unsigned char apple_uuid[] = {	0xf0, 0x72, 0x2e, 0x20, 0x0f, 0x8b, 0x4e, 0x90,
					0x8c, 0xc2, 0x1b, 0x46, 0xf5, 0xf2, 0xef, 0xe2 };

static int add_apple(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root;
	uuid_t root_uuid;
	uint32_t attr783 = 0x00000000;
	uint32_t attr785 = 0x00000002;
	uint16_t attr786 = 0x1234;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_attr_add_new(&record, 0x0780, SDP_UUID128, (void *) apple_uuid);
	sdp_attr_add_new(&record, 0x0781, SDP_TEXT_STR8, (void *) "Macmini");
	sdp_attr_add_new(&record, 0x0782, SDP_TEXT_STR8, (void *) "PowerMac10,1");
	sdp_attr_add_new(&record, 0x0783, SDP_UINT32, (void *) &attr783);
	sdp_attr_add_new(&record, 0x0784, SDP_TEXT_STR8, (void *) "1.6.6f22");
	sdp_attr_add_new(&record, 0x0785, SDP_UINT32, (void *) &attr785);
	sdp_attr_add_new(&record, 0x0786, SDP_UUID16, (void *) &attr786);

	sdp_set_info_attr(&record, "Apple Macintosh Attributes", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("Apple attribute service registered\n");

	return 0;
}

static int add_isync(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass, *proto;
	uuid_t root_uuid, svclass_uuid, serial_uuid, l2cap_uuid, rfcomm_uuid;
	uint8_t channel = si->channel ? si->channel : 16;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(NULL, &l2cap_uuid));

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto = sdp_list_append(proto, sdp_list_append(
		sdp_list_append(NULL, &rfcomm_uuid), sdp_data_alloc(SDP_UINT8, &channel)));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_uuid16_create(&serial_uuid, SERIAL_PORT_SVCLASS_ID);
	svclass = sdp_list_append(NULL, &serial_uuid);

	sdp_uuid16_create(&svclass_uuid, APPLE_AGENT_SVCLASS_ID);
	svclass = sdp_list_append(svclass, &svclass_uuid);

	sdp_set_service_classes(&record, svclass);

	sdp_set_info_attr(&record, "AppleAgent", "Bluetooth acceptor", "Apple Computer Ltd.");

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	printf("Apple iSync service registered\n");

	return 0;
}

static int add_semchla(sdp_session_t *session, svc_info_t *si)
{
	sdp_record_t record;
	sdp_profile_desc_t profile;
	sdp_list_t *root, *svclass, *proto, *profiles;
	uuid_t root_uuid, service_uuid, l2cap_uuid, semchla_uuid;
	uint16_t psm = 0xf0f9;

	memset(&record, 0, sizeof(record));
	record.handle = si->handle;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(
		sdp_list_append(NULL, &l2cap_uuid), sdp_data_alloc(SDP_UINT16, &psm)));

	sdp_uuid32_create(&semchla_uuid, 0x8e770300);
	proto = sdp_list_append(proto, sdp_list_append(NULL, &semchla_uuid));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_uuid32_create(&service_uuid, 0x8e771301);
	svclass = sdp_list_append(NULL, &service_uuid);

	sdp_set_service_classes(&record, svclass);

	sdp_uuid32_create(&profile.uuid, 0x8e771302);	// Headset
	//sdp_uuid32_create(&profile.uuid, 0x8e771303);	// Phone
	profile.version = 0x0100;
	profiles = sdp_list_append(NULL, &profile);
	sdp_set_profile_descs(&record, profiles);

	sdp_set_info_attr(&record, "SEMC HLA", NULL, NULL);

	if (sdp_device_record_register(session, &interface, &record, SDP_RECORD_PERSIST) < 0) {
		printf("Service Record registration failed\n");
		return -1;
	}

	/* SEMC High Level Authentication */
	printf("SEMC HLA service registered\n");

	return 0;
}

static int add_gatt(sdp_session_t *session, svc_info_t *si)
{
	sdp_list_t *svclass_id, *apseq, *proto[2], *profiles, *root, *aproto;
	uuid_t root_uuid, proto_uuid, gatt_uuid, l2cap;
	sdp_profile_desc_t profile;
	sdp_record_t record;
	sdp_data_t *psm, *sh, *eh;
	uint16_t att_psm = 27, start = 0x0001, end = 0x000f;
	int ret;

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = si->handle;
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);
	sdp_list_free(root, NULL);

	sdp_uuid16_create(&gatt_uuid, GENERIC_ATTRIB_SVCLASS_ID);
	svclass_id = sdp_list_append(NULL, &gatt_uuid);
	sdp_set_service_classes(&record, svclass_id);
	sdp_list_free(svclass_id, NULL);

	sdp_uuid16_create(&profile.uuid, GENERIC_ATTRIB_PROFILE_ID);
	profile.version = 0x0100;
	profiles = sdp_list_append(NULL, &profile);
	sdp_set_profile_descs(&record, profiles);
	sdp_list_free(profiles, NULL);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(NULL, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &att_psm);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(NULL, proto[0]);

	sdp_uuid16_create(&proto_uuid, ATT_UUID);
	proto[1] = sdp_list_append(NULL, &proto_uuid);
	sh = sdp_data_alloc(SDP_UINT16, &start);
	proto[1] = sdp_list_append(proto[1], sh);
	eh = sdp_data_alloc(SDP_UINT16, &end);
	proto[1] = sdp_list_append(proto[1], eh);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(NULL, apseq);
	sdp_set_access_protos(&record, aproto);

	sdp_set_info_attr(&record, "Generic Attribute Profile", "BlueZ", NULL);

	sdp_set_url_attr(&record, "http://www.bluez.org/",
			"http://www.bluez.org/", "http://www.bluez.org/");

	sdp_set_service_id(&record, gatt_uuid);

	ret = sdp_device_record_register(session, &interface, &record,
							SDP_RECORD_PERSIST);
	if (ret	< 0)
		printf("Service Record registration failed\n");
	else
		printf("Generic Attribute Profile Service registered\n");

	sdp_data_free(psm);
	sdp_data_free(sh);
	sdp_data_free(eh);
	sdp_list_free(proto[0], NULL);
	sdp_list_free(proto[1], NULL);
	sdp_list_free(apseq, NULL);
	sdp_list_free(aproto, NULL);

	return ret;
}


struct {
	char		*name;
	uint32_t	class;
	int		(*add)(sdp_session_t *sess, svc_info_t *si);
	unsigned char *uuid;
} service[] = {
	{ "DID",	PNP_INFO_SVCLASS_ID,		NULL,		},

	{ "SP",		SERIAL_PORT_SVCLASS_ID,		add_sp		},
	{ "DUN",	DIALUP_NET_SVCLASS_ID,		add_dun		},
	{ "LAN",	LAN_ACCESS_SVCLASS_ID,		add_lan		},
	{ "FAX",	FAX_SVCLASS_ID,			add_fax		},
	{ "OPUSH",	OBEX_OBJPUSH_SVCLASS_ID,	add_opush	},
	{ "FTP",	OBEX_FILETRANS_SVCLASS_ID,	add_ftp		},
	{ "PRINT",	DIRECT_PRINTING_SVCLASS_ID,	add_directprint	},

	{ "HS",		HEADSET_SVCLASS_ID,		add_headset	},
	{ "HSAG",	HEADSET_AGW_SVCLASS_ID,		add_headset_ag	},
	{ "HF",		HANDSFREE_SVCLASS_ID,		add_handsfree	},
	{ "HFAG",	HANDSFREE_AGW_SVCLASS_ID,	add_handsfree_ag},
	{ "SAP",	SAP_SVCLASS_ID,			add_simaccess	},
	{ "PBAP",	PBAP_SVCLASS_ID,		add_pbap,	},

	{ "NAP",	NAP_SVCLASS_ID,			add_nap		},
	{ "GN",		GN_SVCLASS_ID,			add_gn		},
	{ "PANU",	PANU_SVCLASS_ID,		add_panu	},

	{ "HCRP",	HCR_SVCLASS_ID,			NULL		},
	{ "HID",	HID_SVCLASS_ID,			NULL		},
	{ "KEYB",	HID_SVCLASS_ID,			add_hid_keyb	},
	{ "WIIMOTE",	HID_SVCLASS_ID,			add_hid_wiimote	},
	{ "CIP",	CIP_SVCLASS_ID,			add_cip		},
	{ "CTP",	CORDLESS_TELEPHONY_SVCLASS_ID,	add_ctp		},

	{ "A2SRC",	AUDIO_SOURCE_SVCLASS_ID,	add_a2source	},
	{ "A2SNK",	AUDIO_SINK_SVCLASS_ID,		add_a2sink	},
	{ "AVRCT",	AV_REMOTE_SVCLASS_ID,		add_avrct	},
	{ "AVRTG",	AV_REMOTE_TARGET_SVCLASS_ID,	add_avrtg	},

	{ "UDIUE",	UDI_MT_SVCLASS_ID,		add_udi_ue	},
	{ "UDITE",	UDI_TA_SVCLASS_ID,		add_udi_te	},

	{ "SEMCHLA",	0x8e771301,			add_semchla	},

	{ "SR1",	0,				add_sr1,	sr1_uuid	},
	{ "SYNCML",	0,				add_syncml,	syncmlc_uuid	},
	{ "SYNCMLSERV",	0,				NULL,		syncmls_uuid	},
	{ "ACTIVESYNC",	0,				add_activesync,	async_uuid	},
	{ "HOTSYNC",	0,				add_hotsync,	hotsync_uuid	},
	{ "PALMOS",	0,				add_palmos,	palmos_uuid	},
	{ "NOKID",	0,				add_nokiaid,	nokid_uuid	},
	{ "PCSUITE",	0,				add_pcsuite,	pcsuite_uuid	},
	{ "NFTP",	0,				NULL,		nftp_uuid	},
	{ "NSYNCML",	0,				NULL,		nsyncml_uuid	},
	{ "NGAGE",	0,				NULL,		ngage_uuid	},
	{ "APPLE",	0,				add_apple,	apple_uuid	},

	{ "ISYNC",	APPLE_AGENT_SVCLASS_ID,		add_isync,	},
	{ "GATT",	GENERIC_ATTRIB_SVCLASS_ID,	add_gatt,	},

	{ 0 }
};



static int do_search(bdaddr_t *bdaddr, struct search_context *context)
{
	sdp_list_t *attrid, *search, *seq, *next;
	uint32_t range = 0x0000ffff;
	char str[20];
	sdp_session_t *sess;
	int is_nap = 0;

	sess = sdp_connect(&interface, bdaddr, SDP_RETRY_IF_BUSY);
	ba2str(bdaddr, str);
	if (!sess) {
		printf("Failed to connect to SDP server on %s: %s\n", str, strerror(errno));
		return -1;
	}

	attrid = sdp_list_append(0, &range);
	search = sdp_list_append(0, &context->group);

	if (sdp_service_search_attr_req(sess, search, SDP_ATTR_REQ_RANGE, attrid, &seq)) {
		printf("Service Search failed: %s\n", strerror(errno));
		sdp_close(sess);
		return -1;
	}
	sdp_list_free(attrid, 0);
	sdp_list_free(search, 0);

	for (; seq; seq = next) {
		sdp_record_t *rec = (sdp_record_t *) seq->data;
		struct search_context sub_context;

		is_nap = 1;
		//sdp_record_print(rec);
		//printf("tem o servico");
		//print_service_attr(rec);

		if (sdp_get_group_id(rec, &sub_context.group) != -1) {
			/* Set the subcontext for browsing the sub tree */
			memcpy(&sub_context, context, sizeof(struct search_context));
			/* Browse the next level down if not done */
			if (sub_context.group.value.uuid16 != context->group.value.uuid16)
				do_search(bdaddr, &sub_context);
		}

		next = seq->next;
		free(seq);
		sdp_record_free(rec);
	}

	sdp_close(sess);
	return is_nap;
}

static int inquiry(wireless_scan_mi_list *blueap,struct search_context *context)
{
	inquiry_info ii[20];
	uint8_t count = 0;
	int i;
	int is_nap;
	char name[248] = { 0 };
	char str[20];

    int dev_id, sock;


    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );

	struct wireless_scan_mi *current_device;

	if (sdp_general_inquiry(ii, 20, 8, &count) < 0) {
		printf("Inquiry failed\n");
		return -1;
	}

	current_device = blueap->head_list;

	for (i = 0; i < count; i++){

		is_nap = do_search(&ii[i].bdaddr, context);

		if(is_nap){

			current_device = (struct wireless_scan_mi *)malloc(sizeof(struct wireless_scan_mi));

			ba2str(&ii[i].bdaddr, str);

			printf("%s",str);

			strcpy(current_device->mac_address,str);

			if (hci_read_remote_name(sock,&ii[i].bdaddr, sizeof(name),name, 0) < 0)
				strcpy(name, "[unknown]");

			printf("%s ",name);
			strcpy(current_device->essid,name);

			printf("%s\n",is_nap ? "on" : "off");

			blueap->end_list = current_device;

		    current_device = current_device->next;

		}

	}

	current_device = NULL;

	return 0;
}

int scan_bluetooth_service(wireless_scan_mi_list *blueap, char *serv)
{
	struct search_context context;
	unsigned char *uuid = NULL;
	uint32_t class = 0;
	int i;

	/* Initialise context */
	memset(&context, '\0', sizeof(struct search_context));

	/* Note : we need to find a way to support search combining
	 * multiple services */
	context.svc = strdup(serv);

	if (!strncasecmp(context.svc, "0x", 2)) {
		int num;
		/* This is a UUID16, just convert to int */
		sscanf(context.svc + 2, "%X", &num);
		class = num;
		printf("Class 0x%X\n", class);
	} else {
		/* Convert class name to an UUID */

		for (i = 0; service[i].name; i++)
			if (strcasecmp(context.svc, service[i].name) == 0) {
				class = service[i].class;
				uuid = service[i].uuid;
				break;
			}
		if (!class && !uuid) {
			printf("Unknown service %s\n", context.svc);
			return -1;
		}
	}

	if (class) {
		if (class & 0xffff0000)
			sdp_uuid32_create(&context.group, class);
		else {
			uint16_t class16 = class & 0xffff;
			sdp_uuid16_create(&context.group, class16);
		}
	} else
		sdp_uuid128_create(&context.group, uuid);


	return inquiry(blueap,&context);
}

int convertebinario(uint8_t *map, int* map_afh_int)
{
		int i,j; //declaração das variáveis necessárias
		uint8_t dec, q;

	//	for(i=0;i<80;i++) map_afh_int[i] = 0;

		for(i=0;i<10;i++){

			dec = map[i];

			/*Algoritmo para o cálculo */
			for(j = 7 + 8*i; j >= 8*i ; j--)   {
				q = dec / 2;
				map_afh_int[j] = dec % 2;
				dec = q;
			}

		}

		return(0);
}

int cont_badgood_channels(struct wireless_scan_mi *bluenet)
{
		int i; //declaração das variáveis necessárias

		bluenet->good_channels = 0;
		bluenet->bad_channels = 0;


		for(i = 0 ; i < 80 ; i++){

		if(bluenet->map_afh_int[i])
			bluenet->bad_channels = bluenet->bad_channels + 1;
		else
			bluenet->good_channels = bluenet->good_channels + 1;
		}

		return 0;
}

void print_afh_channels(int* map_afh_int){

	int i;

	printf("\nGood channels: ");
	for(i = 1; i <= 79 ; i++){
		if(map_afh_int[i]) printf("%d, ",i);
	}

	printf("\nBad channels: ");
	for(i = 1; i <= 79 ; i++){
			if(!map_afh_int[i]) printf("%d, ",i);
	}

}

int rssiIndBm(int8_t rssi){

	int real_rssi;

	if(rssi < 0)
		real_rssi = (-56+rssi);
	else{
		if(rssi == 0)
			real_rssi = -51;
		else
			real_rssi = -46 + rssi;
	}

	return real_rssi;
}

int connect_scan(int sock, int dev_id)
{
	struct hci_conn_list_req *cl;
	struct hci_conn_info *ci;
	int i;


	char name[248] = { 0 };
	//add
	  // Create conection parameters
	uint16_t handle;

	/*
	uint8_t role;
	unsigned int ptype;

	role = 0x01;
	ptype = HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5;
	*///----

	struct hci_conn_info_req *cr;
	int8_t rssi;
	uint8_t lq;
	int8_t level;

	uint8_t type;
	//uint8_t reason;
	uint8_t mode, map[10];
	int  map_afh_int[80];


	//end add


	if (!(cl = malloc(10 * sizeof(*ci) + sizeof(*cl)))) {
		perror("Can't allocate memory");
		exit(1);
	}
	cl->dev_id = dev_id;
	cl->conn_num = 10;
	ci = cl->conn_info;

	if (ioctl(sock, HCIGETCONNLIST, (void *) cl)) {
		perror("Can't get connection list");
		exit(1);
	}

	printf("\tBDADDR\t\tDevice name\tRSSI\tQuality\t\tTPL\tAFH MAP\n");

	for (i = 0; i < cl->conn_num; i++, ci++) {
		char addr[18];
		char *str;
		ba2str(&ci->bdaddr, addr);
		str = hci_lmtostr(ci->link_mode);

	    //Read name
	    if (hci_read_remote_name(sock, &ci->bdaddr, sizeof(name),
	            name, 0) < 0)
	        strcpy(name, "[unknown]");

	    //Print BD_ADDR and NAME DEVICE
	    printf("%s\t%s\t", addr, name);


	    //Rssi, link, quality, tpl
	    cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
	    if (!cr) {
	      	perror("Can't allocate memory");
	 		exit(1);
	    }

	    bacpy(&cr->bdaddr, &ci->bdaddr);
	    cr->type = ACL_LINK;


	    if (ioctl(sock, HCIGETCONNINFO, (unsigned long) cr) < 0) {
	       	perror("Get connection info failed");
	       	exit(1);
	    }

	    if (hci_read_rssi(sock, htobs(cr->conn_info->handle), &rssi, 1000) < 0) {
	       	perror("Read RSSI failed");
	       	exit(1);
	    }

	    printf("%d dBm\t",rssiIndBm(rssi));

	    if (hci_read_link_quality(sock, htobs(cr->conn_info->handle), &lq, 1000) < 0) {
	       	perror("HCI read_link_quality request failed");
	       	exit(1);
	    }

	    printf("%d\t", lq);

	    if (hci_read_transmit_power_level(sock, htobs(cr->conn_info->handle), type, &level, 1000) < 0) {
	       	perror("HCI read transmit power level request failed");
	 		exit(1);
	    }

	    printf("%s %d\t",(type == 0) ? "Current" : "Maximum", level);

	    handle = htobs(cr->conn_info->handle);

	    if (hci_read_afh_map(sock, handle, &mode, map, 1000) < 0) {
	 		perror("HCI read AFH map request failed");
	 		exit(1);
	 	}

	    if (mode == 0x01) {
	         	int i;
	         	printf("0x");
	         	for (i = 0; i < 10; i++)
	         		printf("%02x", map[i]);
	         		//printf("%02x", map[i]);

	         	printf("\n");

	         	convertebinario(map,map_afh_int);

	         	print_afh_channels(map_afh_int);


	         	printf("\n");

	         } else
	         	printf("AFH disabled\n");

/*
		printf("\t%s %s %s handle %d state %d lm %s\n",
			ci->out ? "<" : ">", type2str(ci->type),
			addr, ci->handle, ci->state, str);
*/


		bt_free(str);
	}

	free(cl);
	return 0;
}

int inquiry_scan(int sock, int dev_id){
    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int len, flags;
    int i;
    char addr[19] = { 0 };
    char name[248] = { 0 };

    // Create conection parameters
	//uint16_t handle;
	//uint8_t role;
	//unsigned int ptype;

	//role = 0x01;
	//ptype = HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5;
    //----
	//struct hci_conn_info_req *cr;
	//int8_t rssi;
	//int8_t lq;
	//int8_t level;

	//uint8_t type;
	//uint8_t reason;
	//uint8_t mode, map[10];
//	int  map_afh_int[80];

	//type =  0;
	//reason =  HCI_OE_USER_ENDED_CONNECTION;

    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");

    printf("\tBDADDR\t\tDevice name\tRSSI\tQuality\t\tTPL\tAFH MAP\n");

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));

        //Read name
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name),
            name, 0) < 0)
        strcpy(name, "[unknown]");

        printf("%s\t%s\t", addr, name);

/*
        //Create connection
        if (hci_create_connection(sock, &(ii+i)->bdaddr, htobs(ptype),
				htobs(0x0000), role, &handle, 25000) < 0)
        	perror("Can't create connection");

        //Rssi, link, quality, tpl
        cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
        if (!cr) {
        	perror("Can't allocate memory");
		exit(1);
        }

        bacpy(&cr->bdaddr, &(ii+i)->bdaddr);
        cr->type = ACL_LINK;


        if (ioctl(sock, HCIGETCONNINFO, (unsigned long) cr) < 0) {
        	perror("Get connection info failed");
        	exit(1);
        }

        if (hci_read_rssi(sock, htobs(cr->conn_info->handle), &rssi, 1000) < 0) {
        	perror("Read RSSI failed");
        	exit(1);
        }

        printf("%d\t", rssi);

        if (hci_read_link_quality(sock, htobs(cr->conn_info->handle), &lq, 1000) < 0) {
        	perror("HCI read_link_quality request failed");
        	exit(1);
        }

        printf("%d\t", lq);

        if (hci_read_transmit_power_level(sock, htobs(cr->conn_info->handle), type, &level, 1000) < 0) {
        	perror("HCI read transmit power level request failed");
		exit(1);
        }

        printf("%s %d\t",
        		(type == 0) ? "Current" : "Maximum", level);

        handle = htobs(cr->conn_info->handle);

        if (hci_read_afh_map(sock, handle, &mode, map, 1000) < 0) {
			perror("HCI read AFH map request failed");
			exit(1);
		}

        if (mode == 0x01) {
        	int i;
        	printf("0x");
        	for (i = 0; i < 10; i++)
        		printf("%02x", map[i]);
        		//printf("%02x", map[i]);

        	printf("\n");

        	convertebinario(map,map_afh_int);

        	print_afh_channels(map_afh_int);


        	printf("\n");

        } else
        	printf("AFH disabled\n");



        if (hci_disconnect(sock, htobs(cr->conn_info->handle),
						reason, 10000) < 0)
        	perror("Disconnect failed");


        free(cr);  */

    }

    free( ii );

	return 0;
}


int iw_scan_bluetooth(wireless_scan_mi_list* list){

    int dev_id, sock;
    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int len, flags;
    int i;
    char addr[19] = { 0 };
    char name[248] = { 0 };

	wireless_scan_mi *current_cell;


    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );

    if (dev_id < 0 || sock < 0) {
    	perror("opening socket");
    	exit(1);
    }

    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);

    if( num_rsp < 0 )
    	printf("Nenhum encontrado");
    	//perror("hci_inquiry");


    if(list->head_list == NULL)
        current_cell = list->head_list;
    else
     	current_cell = list->end_list->next;

   	for (i = 0; i < num_rsp; i++) {

	  	   current_cell = (struct wireless_scan_mi *)malloc(sizeof(struct wireless_scan_mi));

	  	   current_cell->type = 1;

           ba2str(&(ii+i)->bdaddr, addr);
           memset(addr, 0, sizeof(name));

    	        //Read name
    	   if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name),name, 0) < 0)
    		   strcpy(name, "[unknown]");

    	   strcpy(current_cell->essid,name);
    	   strcpy(current_cell->mac_address,addr);

    	   current_cell = current_cell->next;
    }

   	current_cell = NULL;

   	close( sock );

    return 0;

}

int get_myblue_info(wireless_scan_mi* bluenet){

	struct hci_conn_list_req *cl;
	struct hci_conn_info *ci;
	int i;

	char name[248] = { 0 };
	//add
	  // Create conection parameters
	uint16_t handle;

	//----

	struct hci_conn_info_req *cr;
	int8_t rssi;
	uint8_t lq;

	uint8_t mode, map[10];


	//end add
    int dev_id, sock;


    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );

    bluenet->type = 1;

    if (dev_id < 0 || sock < 0) {
    	printf("opening socket");
    	exit(1);
    }


	if (!(cl = malloc(10 * sizeof(*ci) + sizeof(*cl)))) {
		perror("Can't allocate memory");
		exit(1);
	}
	cl->dev_id = dev_id;
	cl->conn_num = 10;
	ci = cl->conn_info;

	if (ioctl(sock, HCIGETCONNLIST, (void *) cl)) {
		perror("Can't get connection list");
		exit(1);
	}


	bluenet->connected = cl->conn_num;

	for (i = 0; i < cl->conn_num; i++, ci++) {
		char addr[18];
		char *str;
		ba2str(&ci->bdaddr, addr);
		str = hci_lmtostr(ci->link_mode);

	    //Read name
	    if (hci_read_remote_name(sock, &ci->bdaddr, sizeof(name),
	            name, 0) < 0)
	        strcpy(name, "[unknown]");

	    //Print BD_ADDR and NAME DEVICE
	    strcpy(bluenet->mac_address,addr);
	    strcpy(bluenet->essid,name);

	    //Rssi, link, quality, tpl
	    cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
	    if (!cr) {
	      	perror("Can't allocate memory");
	 		exit(1);
	    }

	    bacpy(&cr->bdaddr, &ci->bdaddr);
	    cr->type = ACL_LINK;


	    if (ioctl(sock, HCIGETCONNINFO, (unsigned long) cr) < 0) {
	       	perror("Get connection info failed");
	       	bluenet->connected = 0;
	       	exit(1);
	    }



	    if (hci_read_rssi(sock, htobs(cr->conn_info->handle), &rssi, 1000) < 0) {
	       	perror("Read RSSI failed");
	       	exit(1);
	    }

	    bluenet->nivel = rssiIndBm(rssi);

	    if (hci_read_link_quality(sock, htobs(cr->conn_info->handle), &lq, 1000) < 0) {
	       	perror("HCI read_link_quality request failed");
	       	exit(1);
	    }

	    bluenet->qualidade = (float)((lq/255) * 100);

	    handle = htobs(cr->conn_info->handle);

	    if (hci_read_afh_map(sock, handle, &mode, map, 1000) < 0) {
	 		perror("HCI read AFH map request failed");
	 		exit(1);
	 	}

	    if (mode == 0x01) {
	         	convertebinario(map,bluenet->map_afh_int);
	         	cont_badgood_channels(bluenet);
	    }

		bt_free(str);
	}

	free(cl);
	return 0;
};

int test_scan()
{
    int dev_id, sock;


    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        exit(1);
    }

    connect_scan(sock,dev_id);

    //inquiry_scan(int sock, int dev_id);

    close( sock );
    return 0;
}
