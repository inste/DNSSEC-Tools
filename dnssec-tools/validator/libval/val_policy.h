#ifndef VAL_POLICY_H
#define VAL_POLICY_H

#include "val_parse.h"

#define CONF_COMMENT '#'
#define CONF_END_STMT ';'
#define ZONE_COMMENT ';'
#define ZONE_END_STMT '\0'
#define LVL_DELIM ":"
#define MAX_LEVEL_IN_POLICY	5
#define TOKEN_MAX 2048
#define MAX_LINE_SIZE 2048
#define DEFAULT_ZONE	"."


#define P_TRUST_ANCHOR              0
#define P_PREFERRED_SEP             1
#define P_MUST_VERIFY_COUNT         2
#define P_PREFERRED_ALGORITHM_DATA  3
#define P_PREFERRED_ALGORITHM_KEYS  4
#define P_PREFERRED_ALGORITHM_DS    5
#define P_CLOCK_SKEW                6
#define P_EXPIRED_SIGS              7
#define P_USE_TCP                   8
#define P_PROV_UNSECURE             9
#define P_ZONE_SECURITY_EXPECTATION 10
#define P_BASE_LAST                 P_ZONE_SECURITY_EXPECTATION
#ifdef LIBVAL_NSEC3
#define P_NSEC3_MAX_ITER            (P_BASE_LAST+1)
#define NSEC3_POL_COUNT             1
#else
#define NSEC3_POL_COUNT             0
#endif
#ifdef DLV
#define P_DLV_TRUST_POINTS          (P_BASE_LAST+NSEC3_POL_COUNT+1) 
#define P_DLV_MAX_VALIDATION_LINKS  (P_BASE_LAST+NSEC3_POL_COUNT+2) 
#define DLV_POL_COUNT               2
#else
#define DLV_POL_COUNT               0
#endif
#define MAX_POL_TOKEN               (P_BASE_LAST+NSEC3_POL_COUNT+DLV_POL_COUNT+1) 

#define ZONE_PU_TRUSTED_MSG "trusted"
#define ZONE_PU_UNTRUSTED_MSG "untrusted"
#define ZONE_PU_TRUSTED 1
#define ZONE_PU_UNTRUSTED 2

#define ZONE_SE_IGNORE_MSG     "ignore"
#define ZONE_SE_TRUSTED_MSG    "trusted"
#define ZONE_SE_DO_VAL_MSG     "validate"
#define ZONE_SE_UNTRUSTED_MSG  "untrusted"
#define ZONE_SE_IGNORE 1
#define ZONE_SE_TRUSTED 2
#define ZONE_SE_DO_VAL 3
#define ZONE_SE_UNTRUSTED 4

#define RETRIEVE_POLICY(ctx, index, pol) do {\
    pol = (ctx == NULL) ? NULL :\
          (!ctx->e_pol[index])? NULL:(ctx->e_pol[index]);\
    /* check if policies are still the same */\
    if (pol) {\
        policy_entry_t *cur, *next, *prev;\
        struct timeval  tv;\
        /* Get current time */\
        gettimeofday(&tv, NULL);\
        /* Remove expired policies */\
        prev = NULL;\
        cur = pol;\
        while (cur) {\
            next = cur->next;\
            if (cur->exp_ttl > 0 &&\
                cur->exp_ttl <= tv.tv_sec) {\
                if (prev) {\
                    prev->next = next;\
                } else {\
                    pol = next;\
                }\
                cur->next = NULL;\
                free_policy_entry(cur, index);\
                cur = next;\
            } else {\
                prev = cur;\
                cur = cur->next;\
            }\
        }\
    }\
} while(0)
    

char           *resolv_conf_get(void);
int             resolv_conf_set(const char *name);

char           *root_hints_get(void);
int             root_hints_set(const char *name);

char           *dnsval_conf_get(void);
int             dnsval_conf_set(const char *name);


int             read_root_hints_file(val_context_t * ctx);
int             read_res_config_file(val_context_t * ctx);
int             read_val_config_file(val_context_t * ctx, char *scope);
void            destroy_valpol(val_context_t * ctx);
void            destroy_respol(val_context_t * ctx);
struct hosts   *parse_etc_hosts(const char *name);

int             parse_trust_anchor(char **, char *, policy_entry_t *, int *, int *);
int             free_trust_anchor(policy_entry_t *);
int             parse_preferred_sep(char **, char *, policy_entry_t *, int *, int *);
int             free_preferred_sep(policy_entry_t *);
int             parse_must_verify_count(char **, char *, policy_entry_t *, int *, int *);
int             free_must_verify_count(policy_entry_t *);
int             parse_preferred_algo_data(char **, char *, policy_entry_t *, int *, int *);
int             free_preferred_algo_data(policy_entry_t *);
int             parse_preferred_algo_keys(char **, char *, policy_entry_t *, int *, int *);
int             free_preferred_algo_keys(policy_entry_t *);
int             parse_preferred_algo_ds(char **, char *, policy_entry_t *, int *, int *);
int             free_preferred_algo_ds(policy_entry_t *);
int             parse_clock_skew(char **, char *, policy_entry_t *, int *, int *);
int             free_clock_skew(policy_entry_t *);
int             parse_expired_sigs(char **, char *, policy_entry_t *, int *, int *);
int             free_expired_sigs(policy_entry_t *);
int             parse_use_tcp(char **, char *, policy_entry_t *, int *, int *);
int             free_use_tcp(policy_entry_t *);
int             parse_prov_unsecure_status(char **, char *, policy_entry_t *, int *, int *);
int             free_prov_unsecure_status(policy_entry_t *);
int             parse_zone_security_expectation(char **, char *, policy_entry_t *, int *, int *);
int             free_zone_security_expectation(policy_entry_t *);
#ifdef LIBVAL_NSEC3
int             parse_nsec3_max_iter(char **, char *, policy_entry_t * pol_entry, int *line_number, int *);
int             free_nsec3_max_iter(policy_entry_t * pol_entry);
#endif
#ifdef DLV
int             parse_dlv_trust_points(char **, char *, policy_entry_t *, int *, int *);
int             free_dlv_trust_points(policy_entry_t *);
int             parse_dlv_max_links(char **, char *, policy_entry_t *, int *, int *);
int             free_dlv_max_links(policy_entry_t *);
#endif
int             check_relevance(char *label, char *scope, int *label_count,
                                int *relevant);
int             val_add_valpolicy(val_context_t *ctx, char *keyword, char *zone,
                                  char *value, long ttl);
int             val_get_token(char **buf_ptr,
                              char *end_ptr,
                              int *line_number,
                              char *conf_token,
                              int conf_limit,
                              int *endst, char comment_c, char endstmt_c);
int free_policy_entry(policy_entry_t *pol_entry, int index);

/*
 * fragment of the configuration file containing 
 * one policy chunk
 */
struct policy_fragment {
    char           *label;
    int             label_count;
    int             index;
    policy_entry_t  *pol;
};

struct policy_conf_element {
    const char     *keyword;
    int             (*parse) (char **, char *, policy_entry_t *, int *, int *);
    int             (*free) (policy_entry_t *);
};

extern const struct policy_conf_element conf_elem_array[];

struct trust_anchor_policy {
    val_dnskey_rdata_t *publickey;
};

struct clock_skew_policy {
    long             clock_skew;
};

struct prov_unsecure_policy {
    int             trusted;
};

struct zone_se_policy {
    int             trusted;
};


#ifdef LIBVAL_NSEC3
struct nsec3_max_iter_policy {
    int             iter;
};
#endif

#endif                          /* VAL_POLICY_H */
