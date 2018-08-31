#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ucimap.h>
#include <stdint.h>
#include "list.h"

#define __in
#define __inout
#define APX_PRIORITY_COUNT 8
typedef int      APX_STATUS;
typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t  UINT8;
typedef struct _APX_ENGINE APX_ENGINE;
typedef enum BOOL { FALSE = 0, TRUE = 1 } BOOL;
#include "appexEngineClsfConfig.h"

#define APX_UCI_OPTION(s, v, p) { UCIMAP_OPTION(s, v), .type = p, .name = #v }

static struct list_head g_profile_list;
static struct list_head g_filter_list;
static struct uci_sectionmap g_profile_map;

/*------------------------------------------------------------------------*/

typedef struct uci_ClsfProfile
{
	struct ucimap_section_data map;
	struct list_head list;

    uint32_t    ID;
	char        *Name;
    uint32_t    Action;
    uint32_t    ActionExclude;
    char        *Priority;

    char        *SrcIpFilter;
    char        *DstIpFilter;
    char        *SrcPortFilter;
    char        *DstPortFilter;
    char        *ProtoFilter;
    uint16_t    L7Filter;

    uint16_t    Engine;
} uci_ClsfProfile;

static int
network_init_ClsfProfile(struct uci_map *map, void *section, struct uci_section *s)
{
	uci_ClsfProfile *profile = section;
    unsigned long offset = (unsigned long)&((uci_ClsfProfile*)0)->ID;
    memset((uint8_t*)profile + offset, 0, sizeof(*profile) - offset);
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
    APX_UCI_OPTION(uci_ClsfProfile, Name, UCIMAP_STRING),
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

	char        *Name;
	char        *Rules;
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
    APX_UCI_OPTION(uci_ClsfFilter, Name, UCIMAP_STRING),
    APX_UCI_OPTION(uci_ClsfFilter, Rules, UCIMAP_STRING),
};

static struct uci_sectionmap g_filter_map =
{
	UCIMAP_SECTION(uci_ClsfFilter, map),
	.type = "ObjMgmtIp",
	.init = network_init_ClsfFilter,
	.add = network_add_ClsfFilter,
	.options = g_filter_map_options,
	.n_options = ARRAY_SIZE(g_filter_map_options),
};

/*------------------------------------------------------------------------*/

static struct uci_sectionmap *network_smap[] =
{
	&g_profile_map,
	&g_filter_map,
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
	struct list_head *p;
	uci_ClsfProfile *profile;
    uci_ClsfFilter *filter;

	INIT_LIST_HEAD(&g_profile_list);
	INIT_LIST_HEAD(&g_filter_list);

	ctx = uci_alloc_context();

	uci_set_confdir(ctx, "./test/config");
	uci_load(ctx, "appex", &pkg);

	ucimap_init(&clsf_map);
	ucimap_parse(&clsf_map, pkg);

	list_for_each(p, &g_profile_list)
    {
		profile = list_entry(p, struct uci_ClsfProfile, list);
        printf("\t ID = 0x%x\n", profile->ID);
        if (profile->Name) printf("\t Name = %s\n", profile->Name);
        printf("\t Action = 0x%x\n", profile->Action);
        printf("\t ActionExclude = 0x%x\n", profile->ActionExclude);
        if (profile->Priority) printf("\t Priority = %s\n", profile->Priority);
        if (profile->SrcIpFilter) printf("\t SrcIpFilter = %s\n", profile->SrcIpFilter);
        if (profile->DstIpFilter) printf("\t DstIpFilter = %s\n", profile->DstIpFilter);
        if (profile->SrcPortFilter) printf("\t SrcPortFilter = %s\n", profile->SrcPortFilter);
        if (profile->DstPortFilter) printf("\t DstPortFilter = %s\n", profile->DstPortFilter);
        if (profile->ProtoFilter) printf("\t ProtoFilter = %s\n", profile->ProtoFilter);
        printf("\t L7Filter = %u\n", profile->L7Filter);
        printf("\t Engine = 0x%x\n", profile->Engine);
	}

	list_for_each(p, &g_filter_list)
    {
		filter = list_entry(p, struct uci_ClsfFilter, list);
        printf("Rules: %s\n", filter->Rules);
    }

	ucimap_cleanup(&clsf_map);
	uci_free_context(ctx);

	return 0;
}
