#define _GNU_SOURCE
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ucimap.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "list.h"

#define __in
#define __inout
#define APX_PRIORITY_COUNT 8
typedef int		 APX_STATUS;
typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t	 UINT8;
typedef int32_t	 INT32;
#include "appexEngineClsfConfig.h"
#include "appexPlatformConfig.h"

#define APX_OFFSET_OF(_s_, _f_) ((size_t)&((_s_*)0)->_f_)

#define APX_UCI_OPTION(s, v, p) { UCIMAP_OPTION(s, v), .type = p, .name = #v }

static struct list_head g_profile_list;
static struct list_head g_filter_list;
static struct uci_sectionmap g_profile_map;

void apply_clsf_cfg(int engine);

/*------------------------------------------------------------------------*/

typedef struct uci_ClsfProfile
{
	struct ucimap_section_data map;
	struct list_head list;

	uint32_t	ID;
	uint32_t	Disable;
	char		*Name;
	uint32_t	Action;
	uint32_t	ActionExclude;
	char		*Priority;

	char		*SrcIpFilter;
	char		*DstIpFilter;
	char		*SrcPortFilter;
	char		*DstPortFilter;
	char		*ProtoFilter;
	uint32_t	L7Filter;

	uint32_t	Engine;
} uci_ClsfProfile;

static int
network_init_ClsfProfile(struct uci_map *map, void *section, struct uci_section *s)
{
	uci_ClsfProfile *profile = section;
	unsigned long offset = (unsigned long)&((uci_ClsfProfile*)0)->ID;
	memset((uint8_t*)profile + offset, 0, sizeof(*profile) - offset);
	profile->Name = s->e.name;
	return 0;
}

static int
network_add_ClsfProfile(struct uci_map *map, void *section)
{
	uci_ClsfProfile *profile = section;
	list_add_tail(&profile->list, &g_profile_list);
	return 0;
}

static struct uci_optmap g_profile_map_options[] =
{
	APX_UCI_OPTION(uci_ClsfProfile, ID, UCIMAP_INT),
	APX_UCI_OPTION(uci_ClsfProfile, Disable, UCIMAP_INT),
	APX_UCI_OPTION(uci_ClsfProfile, Action, UCIMAP_INT),
	APX_UCI_OPTION(uci_ClsfProfile, ActionExclude, UCIMAP_INT),
	APX_UCI_OPTION(uci_ClsfProfile, Priority, UCIMAP_STRING),
	APX_UCI_OPTION(uci_ClsfProfile, SrcIpFilter, UCIMAP_STRING),
	APX_UCI_OPTION(uci_ClsfProfile, DstIpFilter, UCIMAP_STRING),
	APX_UCI_OPTION(uci_ClsfProfile, SrcPortFilter, UCIMAP_STRING),
	APX_UCI_OPTION(uci_ClsfProfile, DstPortFilter, UCIMAP_STRING),
	APX_UCI_OPTION(uci_ClsfProfile, ProtoFilter, UCIMAP_STRING),
	APX_UCI_OPTION(uci_ClsfProfile, L7Filter, UCIMAP_INT),
	APX_UCI_OPTION(uci_ClsfProfile, Engine, UCIMAP_INT),
};

static struct uci_sectionmap g_profile_map =
{
	UCIMAP_SECTION(uci_ClsfProfile, map),
	.type = "ClsfProfile",
	.init = network_init_ClsfProfile,
	.add = network_add_ClsfProfile,
	.options = g_profile_map_options,
	.n_options = ARRAY_SIZE(g_profile_map_options),
};

/*------------------------------------------------------------------------*/

typedef struct uci_ClsfFilter
{
	struct ucimap_section_data map;
	struct list_head list;

	char		*Name;
	char		*Rules;
} uci_ClsfFilter;

static int
network_init_ClsfFilter(struct uci_map *map, void *section, struct uci_section *s)
{
	uci_ClsfFilter *filter = section;
	filter->Rules = NULL;
	filter->Name = s->e.name;
	return 0;
}

static int
network_add_ClsfFilter(struct uci_map *map, void *section)
{
	uci_ClsfFilter *filter = section;
	list_add_tail(&filter->list, &g_filter_list);
	return 0;
}

static struct uci_optmap g_filter_map_options[] =
{
	APX_UCI_OPTION(uci_ClsfFilter, Rules, UCIMAP_STRING),
};

static struct uci_sectionmap g_ip_filter_map =
{
	UCIMAP_SECTION(uci_ClsfFilter, map),
	.type = "ObjMgmtIp",
	.init = network_init_ClsfFilter,
	.add = network_add_ClsfFilter,
	.options = g_filter_map_options,
	.n_options = ARRAY_SIZE(g_filter_map_options),
};

static struct uci_sectionmap g_port_filter_map =
{
	UCIMAP_SECTION(uci_ClsfFilter, map),
	.type = "ObjMgmtPort",
	.init = network_init_ClsfFilter,
	.add = network_add_ClsfFilter,
	.options = g_filter_map_options,
	.n_options = ARRAY_SIZE(g_filter_map_options),
};

/*------------------------------------------------------------------------*/

static struct uci_sectionmap *network_smap[] =
{
	&g_profile_map,
	&g_ip_filter_map,
	&g_port_filter_map,
};

static struct uci_map clsf_map =
{
	.sections = network_smap,
	.n_sections = ARRAY_SIZE(network_smap),
};

int main(int argc, char **argv)
{
	struct uci_context *ctx;
	struct uci_package *pkg;

	INIT_LIST_HEAD(&g_profile_list);
	INIT_LIST_HEAD(&g_filter_list);

	ctx = uci_alloc_context();

	uci_set_confdir(ctx, "/home/lsun/download/uci-2010-09-28.1/clsf/config/");
	uci_load(ctx, "appex", &pkg);

	ucimap_init(&clsf_map);
	ucimap_parse(&clsf_map, pkg);

	apply_clsf_cfg(0);
	apply_clsf_cfg(1);
	apply_clsf_cfg(2);
	apply_clsf_cfg(3);

	ucimap_cleanup(&clsf_map);
	uci_free_context(ctx);

	return 0;
}

struct _IoBuf
{
	APX_BASE_IOCTL_INFO		Header;
	union
	{
		APX_CLSF_CFG		ClsfCfg;
		APX_CLSF_STATS		ClsfStats;
		UINT8				Buf[16000];
	};
} ioBuf;

char* find_rule(char *name)
{
	struct list_head *p;
	uci_ClsfFilter *filter;

	list_for_each(p, &g_filter_list)
	{
		filter = list_entry(p, struct uci_ClsfFilter, list);
		if (strcasecmp(filter->Name, name) == 0)
		{
			return filter->Rules;
		}
	}

	printf("rule %s not found\n", name);
	exit(-1);
	return NULL;
}

int add_rules(APX_CLSF_FILTER_CFG *filter, uint32_t type, char *rules)
{
	int num = 0, len;
	char buf[64], *p, *p_dash, *p_slash;

	while (sscanf(rules, "%s", buf) == 1)
	{
		p = buf;
		len = strlen(buf);
		p_dash = strchr(buf, '-');
		if (p_dash) *p_dash++ = 0;

		p_slash = strchr(buf, '/');
		if (p_slash) *p_slash++ = 0;

		filter[num].Field = type;

		switch (type)
		{
			case APX_CLSF_FILTER_SRC_IP:
			case APX_CLSF_FILTER_DST_IP:
				filter[num].u.Ip.Val1 = inet_network(p);
				if (p_dash) filter[num].u.Ip.Val2 = inet_network(p_dash);
				else if (p_slash) {
					int mask = atoi(p_slash);
					filter[num].u.Ip.Val2 = filter[num].u.Ip.Val1 | ((1 << (32 - mask)) - 1);
				}
				else filter[num].u.Ip.Val2 = filter[num].u.Ip.Val1;
				break;

			case APX_CLSF_FILTER_SRC_PORT:
			case APX_CLSF_FILTER_DST_PORT:
				filter[num].u.Port.Val1 = atoi(p);
				if (p_dash) filter[num].u.Port.Val2 = atoi(p_dash);
				else filter[num].u.Port.Val2 = filter[num].u.Port.Val1;
				break;

			case APX_CLSF_FILTER_PROTO:
				filter[num].u.Protocol.Val = atoi(p);
				break;
		}

		while (*rules == ' ' || *rules == '\t') rules++;
		rules += len;
		while (*rules == ' ' || *rules == '\t') rules++;
		num++;
	}

	return num;
}

int appexIoctl(APX_BASE_IOCTL_INFO *info, int engId, int set)
{
	char path[128];
	int fd = 0;

return 0;
	if (engId)
	{	
		sprintf(path, "/proc/net/appex%d/ioctl", engId);
	}	
	else
	{	
		sprintf(path, "/proc/net/appex/ioctl");
	}	

	if ((fd = open(path, O_RDWR)) < 0)
	{	
		perror("");
		return -1; 
	}	

	if(ioctl(fd, set? APX_IOCTL_SET : APX_IOCTL_GET, (unsigned long)info) == -1) 
	{	
		perror("ioctl: ");
		return -1; 
	}	

	close(fd);
	
	return 0;
}

void apply_clsf_cfg(int engine)
{
	struct list_head *node;
	uci_ClsfProfile *profile;
	uci_ClsfFilter *filter;
	APX_CLSF_VIEW_CFG *viewCfg;
	APX_CLSF_PROFILE_CFG *profileCfg;
	APX_CLSF_CFG *Cfg = &ioBuf.ClsfCfg;
	UINT8 *p = Cfg->Cfg;
	char buf[64], *tmp, *rules;

	memset(&ioBuf, 0, sizeof(ioBuf));

	list_for_each(node, &g_profile_list)
	{
		profileCfg = (APX_CLSF_PROFILE_CFG*)p;
		profile = list_entry(node, struct uci_ClsfProfile, list);
                
		if (profile->Disable) continue;

		if (!(profile->Engine & (1 << engine))) continue;

		profileCfg->ID = APX_CLSF_PROFILE_ID(profile->ID);
		if (profile->Name)
		{
			strncpy(profileCfg->Name, profile->Name, sizeof(profileCfg->Name) - 1);
		}
		profileCfg->Action = profile->Action;
		profileCfg->ActionExclude = profile->ActionExclude;
		if (profile->Priority)
		{
			sscanf(profile->Priority, "%d %d", &profileCfg->Priority[0], &profileCfg->Priority[1]);
		}

		profileCfg->NumFilters = 0;

		if (profile->SrcIpFilter)
		{
			tmp = profile->SrcIpFilter;
			while (sscanf(tmp, "%s", buf) == 1)
			{
				rules = find_rule(buf);
				profileCfg->NumFilters += add_rules(&profileCfg->Filters[profileCfg->NumFilters],
					APX_CLSF_FILTER_SRC_IP, rules);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
				tmp += strlen(buf);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
			}
		}
		if (profile->DstIpFilter)
		{
			tmp = profile->DstIpFilter;
			while (sscanf(tmp, "%s", buf) == 1)
			{
				rules = find_rule(buf);
				profileCfg->NumFilters += add_rules(&profileCfg->Filters[profileCfg->NumFilters],
					APX_CLSF_FILTER_DST_IP, rules);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
				tmp += strlen(buf);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
			}
		}
		if (profile->SrcPortFilter)
		{
			tmp = profile->SrcPortFilter;
			while (sscanf(tmp, "%s", buf) == 1)
			{
				rules = find_rule(buf);
				profileCfg->NumFilters += add_rules(&profileCfg->Filters[profileCfg->NumFilters],
					APX_CLSF_FILTER_SRC_PORT, rules);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
				tmp += strlen(buf);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
			}
		}
		if (profile->DstPortFilter)
		{
			tmp = profile->DstPortFilter;
			while (sscanf(tmp, "%s", buf) == 1)
			{
				rules = find_rule(buf);
				profileCfg->NumFilters += add_rules(&profileCfg->Filters[profileCfg->NumFilters],
					APX_CLSF_FILTER_DST_PORT, rules);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
				tmp += strlen(buf);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
			}
		}
		if (profile->ProtoFilter)
		{
			tmp = profile->ProtoFilter;
			while (sscanf(tmp, "%s", buf) == 1)
			{
				rules = buf;
				profileCfg->NumFilters += add_rules(&profileCfg->Filters[profileCfg->NumFilters],
					APX_CLSF_FILTER_PROTO, rules);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
				tmp += strlen(buf);
				while (*tmp == ' ' || *tmp == '\t') tmp++;
			}
		}
		if (profile->L7Filter)
		{
			profileCfg->Filters[profileCfg->NumFilters].Field = APX_CLSF_FILTER_L7RULE;
			profileCfg->Filters[profileCfg->NumFilters].u.L7Rule.ID = profile->L7Filter;
			profileCfg->NumFilters++;
		}

		p += APX_CLSF_PROFILE_CFG_SIZE(profileCfg);
	}

	viewCfg = (APX_CLSF_VIEW_CFG*)p;
	viewCfg->ID = APX_CLSF_VIEW_ID(1);
	viewCfg->DefaultPriority[0] = 1;
	viewCfg->DefaultPriority[1] = 1;
	viewCfg->DefaultAction = 0;
	strcpy(viewCfg->Name, "view");
	viewCfg->Active = 1;
	viewCfg->LooseMapping = 0;
	list_for_each(node, &g_profile_list)
	{
		profile = list_entry(node, struct uci_ClsfProfile, list);
		if (!(profile->Engine & (1 << engine))) continue;
		viewCfg->ProfileGroupIDs[viewCfg->NumProfilesGroups] = APX_CLSF_PROFILE_ID(profile->ID);
		viewCfg->NumProfilesGroups++;
	}
	p += APX_CLSF_VIEW_CFG_SIZE(viewCfg);

	if (viewCfg->NumProfilesGroups == 0) return;

	Cfg->Type = APX_CLSF_CFG_TYPE_VIEW;
	Cfg->Len = p - Cfg->Cfg;

	ioBuf.Header.Len = APX_CLSF_CFG_SIZE(Cfg);
	ioBuf.Header.Type = APX_BASE_IOCTL_CLSF_CFG;
	appexIoctl(&ioBuf.Header, engine, 1);
}
