
#include <math.h>

#include "rpl.h"
#include "../system.h"
#include "../measure.h"


    /**** global variables ****/

uint16                      rpl_event_id_after_node_wake;
uint16                      rpl_event_id_before_node_kill;

uint16                      rpl_event_id_after_dis_pdu_sent;
uint16                      rpl_event_id_after_dis_pdu_received;
uint16                      rpl_event_id_after_dio_pdu_sent;
uint16                      rpl_event_id_after_dio_pdu_received;
uint16                      rpl_event_id_after_dao_pdu_sent;
uint16                      rpl_event_id_after_dao_pdu_received;

uint16                      rpl_event_id_after_neighbor_attach;
uint16                      rpl_event_id_after_neighbor_detach;

uint16                      rpl_event_id_after_forward_failure;
uint16                      rpl_event_id_after_forward_error;

uint16                      rpl_event_id_after_trickle_timer_t_timeout;
uint16                      rpl_event_id_after_trickle_timer_i_timeout;
uint16                      rpl_event_id_after_seq_num_timer_timeout;


    /**** local function prototypes ****/

static rpl_neighbor_t *     rpl_neighbor_create(node_t *node);
static void                 rpl_neighbor_destroy(rpl_neighbor_t* neighbor);

static rpl_root_info_t *    rpl_root_info_create();
static void                 rpl_root_info_destroy(rpl_root_info_t *root_info);

static rpl_dodag_t *        rpl_dodag_create(rpl_dio_pdu_t *dio_pdu);
static void                 rpl_dodag_destroy(rpl_dodag_t *dodag);

static bool                 dio_pdu_changed(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu);
static bool                 dio_pdu_dodag_id_changed(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu);
static bool                 dio_pdu_dodag_seq_num_changed(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu);
static bool                 dio_pdu_dodag_rank_increased(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu);
static bool                 dio_pdu_dodag_config_changed(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu);

static void                 start_dio_poisoning(node_t *node);
static void                 start_as_root(node_t *node);
static void                 join_dodag_iteration(node_t *node, rpl_dio_pdu_t *dio_pdu);

static bool                 choose_parents_and_siblings(node_t *node);
static rpl_dio_pdu_t *      get_preferred_dodag_dio_pdu(node_t *node, bool *same);
static void                 update_dodag_config(node_t *node, rpl_dio_pdu_t *dio_pdu);
static void                 reset_trickle_timer(node_t *node);

static rpl_dio_pdu_t *      create_current_dio_message(node_t *node, bool include_dodag_config);
static void                 update_neighbor_dio_message(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu);

static int8                 compare_ranks(uint8 rank1, uint8 rank2, uint8 min_hop_rank_inc);
static uint8                compute_candidate_rank(node_t *node, rpl_neighbor_t *neighbor);

static void                 event_arg_str_one_node_func(void *data1, void *data2, char *str1, char *str2, uint16 len);


    /**** exported functions ****/

bool rpl_init()
{
    rpl_event_id_after_node_wake = event_register("after_node_wake", "rpl", (event_handler_t) rpl_event_after_node_wake, NULL);
    rpl_event_id_before_node_kill = event_register("before_node_kill", "rpl", (event_handler_t) rpl_event_before_node_kill, NULL);

    rpl_event_id_after_dis_pdu_sent = event_register("after_dis_pdu_sent", "rpl", (event_handler_t) rpl_event_after_dis_pdu_sent, event_arg_str_one_node_func);
    rpl_event_id_after_dis_pdu_received = event_register("after_dis_pdu_received", "rpl", (event_handler_t) rpl_event_after_dis_pdu_received, event_arg_str_one_node_func);
    rpl_event_id_after_dio_pdu_sent = event_register("after_dio_pdu_sent", "rpl", (event_handler_t) rpl_event_after_dio_pdu_sent, event_arg_str_one_node_func);
    rpl_event_id_after_dio_pdu_received = event_register("after_dio_pdu_received", "rpl", (event_handler_t) rpl_event_after_dio_pdu_received, event_arg_str_one_node_func);
    rpl_event_id_after_dao_pdu_sent = event_register("after_dao_pdu_sent", "rpl", (event_handler_t) rpl_event_after_dao_pdu_sent, event_arg_str_one_node_func);
    rpl_event_id_after_dao_pdu_received = event_register("after_dao_pdu_received", "rpl", (event_handler_t) rpl_event_after_dao_pdu_received, NULL);

    rpl_event_id_after_neighbor_attach = event_register("after_neighbor_attach", "rpl", (event_handler_t) rpl_event_after_neighbor_attach, event_arg_str_one_node_func);
    rpl_event_id_after_neighbor_detach = event_register("after_neighbor_detach", "rpl", (event_handler_t) rpl_event_after_neighbor_detach, event_arg_str_one_node_func);

    rpl_event_id_after_forward_failure = event_register("after_forward_failure", "rpl", (event_handler_t) rpl_event_after_forward_failure, NULL);
    rpl_event_id_after_forward_error = event_register("after_forward_error", "rpl", (event_handler_t) rpl_event_after_forward_error, NULL);

    rpl_event_id_after_trickle_timer_t_timeout = event_register("after_trickle_timer_t_timeout", "rpl", (event_handler_t) rpl_event_after_trickle_timer_t_timeout, NULL);
    rpl_event_id_after_trickle_timer_i_timeout = event_register("after_trickle_timer_i_timeout", "rpl", (event_handler_t) rpl_event_after_trickle_timer_i_timeout, NULL);
    rpl_event_id_after_seq_num_timer_timeout = event_register("after_seq_num_timer_timeout", "rpl", (event_handler_t) rpl_event_after_seq_num_timer_timeout, NULL);

    return TRUE;
}

bool rpl_done()
{
    return TRUE;
}

rpl_dio_pdu_t *rpl_dio_pdu_create()
{
    rpl_dio_pdu_t *pdu = malloc(sizeof(rpl_dio_pdu_t));

    pdu->dodag_id = NULL;
    pdu->dodag_pref = RPL_DEFAULT_DAG_PREF;
    pdu->seq_num = 0;

    pdu->rank = RPL_RANK_INFINITY;
    pdu->dstn = 0;
    pdu->dao_stored = FALSE;

    pdu->grounded = FALSE;
    pdu->dao_supported = rs_system->rpl_dao_supported;
    pdu->dao_trigger = FALSE;

    pdu->dodag_config_suboption = NULL;

    return pdu;
}

void rpl_dio_pdu_destroy(rpl_dio_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    if (pdu->dodag_id != NULL)
        free(pdu->dodag_id);

    if (pdu->dodag_config_suboption != NULL)
        rpl_dio_suboption_dodag_config_destroy(pdu->dodag_config_suboption);

    free(pdu);
}

rpl_dio_pdu_t *rpl_dio_pdu_duplicate(rpl_dio_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    rpl_dio_pdu_t *new_pdu = malloc(sizeof(rpl_dio_pdu_t));

    new_pdu->dodag_id = strdup(pdu->dodag_id);
    new_pdu->dodag_pref = pdu->dodag_pref;
    new_pdu->seq_num = pdu->seq_num;

    new_pdu->rank = pdu->rank;
    new_pdu->dstn = pdu->dstn;
    new_pdu->dao_stored = pdu->dao_stored;

    new_pdu->grounded = pdu->grounded;
    new_pdu->dao_supported = pdu->dao_supported;
    new_pdu->dao_trigger = pdu->dao_trigger;

    if (pdu->dodag_config_suboption != NULL) {
        new_pdu->dodag_config_suboption = rpl_dio_suboption_dodag_config_create();

        new_pdu->dodag_config_suboption->dio_interval_doublings = pdu->dodag_config_suboption->dio_interval_doublings;
        new_pdu->dodag_config_suboption->dio_interval_min = pdu->dodag_config_suboption->dio_interval_min;
        new_pdu->dodag_config_suboption->dio_redundancy_constant = pdu->dodag_config_suboption->dio_redundancy_constant;
        new_pdu->dodag_config_suboption->max_rank_inc = pdu->dodag_config_suboption->max_rank_inc;
        new_pdu->dodag_config_suboption->min_hop_rank_inc = pdu->dodag_config_suboption->min_hop_rank_inc;
    }
    else {
        new_pdu->dodag_config_suboption = NULL;
    }

    return new_pdu;
}

rpl_dio_suboption_dodag_config_t *rpl_dio_suboption_dodag_config_create()
{
    rpl_dio_suboption_dodag_config_t *suboption = malloc(sizeof(rpl_dio_suboption_dodag_config_t));

    suboption->dio_interval_doublings = 0;
    suboption->dio_interval_min = 0;
    suboption->dio_redundancy_constant = 0;
    suboption->max_rank_inc = 0;
    suboption->min_hop_rank_inc = 0;

    return suboption;
}

void rpl_dio_suboption_dodag_config_destroy(rpl_dio_suboption_dodag_config_t *suboption)
{
    rs_assert(suboption != NULL);

    free(suboption);
}

rpl_dao_pdu_t *rpl_dao_pdu_create()
{
    rpl_dao_pdu_t *pdu = malloc(sizeof(rpl_dao_pdu_t));

    pdu->seq_num = 0;
    pdu->rank = 0;
    pdu->dest = NULL;
    pdu->prefix_len = 0;
    pdu->life_time = 0;
    pdu->rr_stack = NULL;
    pdu->rr_count = 0;

    return pdu;
}

void rpl_dao_pdu_destroy(rpl_dao_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    while (pdu->rr_count > 0) {
        free(pdu->rr_stack[--pdu->rr_count]);
    }

    free(pdu->rr_stack);

    free(pdu);
}

rpl_dao_pdu_t *rpl_dao_pdu_duplicate(rpl_dao_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    rpl_dao_pdu_t *new_pdu = malloc(sizeof(rpl_dao_pdu_t));

    new_pdu->seq_num = pdu->seq_num;
    new_pdu->rank = pdu->rank;
    new_pdu->dest = strdup(pdu->dest);
    new_pdu->prefix_len = pdu->prefix_len;
    new_pdu->life_time = pdu->life_time;

    if (pdu->rr_count == 0) {
        new_pdu->rr_stack = NULL;
    }
    else {
        new_pdu->rr_stack = malloc(pdu->rr_count * sizeof(char *));

        int i;
        for (i = 0; i < pdu->rr_count; i++) {
            new_pdu->rr_stack[i] = strdup(pdu->rr_stack[i]);
        }
    }

    new_pdu->rr_count = pdu->rr_count;

    return new_pdu;
}

void rpl_dao_pdu_add_rr(rpl_dao_pdu_t *pdu, char *ip_address)
{
    rs_assert(pdu != NULL);
    rs_assert(ip_address != NULL);

    pdu->rr_stack = realloc(pdu->rr_stack, (++pdu->rr_count) * sizeof(char *));
    pdu->rr_stack[pdu->rr_count - 1] = strdup(ip_address);
}

bool rpl_node_init(node_t *node)
{
    rs_assert(node != NULL);

    node->rpl_info = malloc(sizeof(rpl_node_info_t));

    node->rpl_info->root_info = rpl_root_info_create();
    node->rpl_info->joined_dodag = NULL;

    node->rpl_info->storing = RPL_DEFAULT_NODE_STORING;
    node->rpl_info->trickle_i_doublings_so_far = 0;
    node->rpl_info->trickle_i = 0;
    node->rpl_info->trickle_c = 0;

    node->rpl_info->neighbor_list = NULL;
    node->rpl_info->neighbor_count = 0;

    node->rpl_info->poison_count_so_far = 0;

    return TRUE;
}

void rpl_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->rpl_info != NULL) {
        rpl_node_remove_all_neighbors(node);

        if (node->rpl_info->root_info != NULL) {
            rpl_root_info_destroy(node->rpl_info->root_info);
        }

        if (node->rpl_info->joined_dodag) {
            rpl_dodag_destroy(node->rpl_info->joined_dodag);
        }

        free(node->rpl_info);
    }
}

void rpl_node_add_neighbor(node_t *node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    rpl_neighbor_t *neighbor = rpl_neighbor_create(neighbor_node);

    node->rpl_info->neighbor_list = realloc(node->rpl_info->neighbor_list, (node->rpl_info->neighbor_count + 1) * sizeof(rpl_neighbor_t *));
    node->rpl_info->neighbor_list[node->rpl_info->neighbor_count++] = neighbor;
}

bool rpl_node_remove_neighbor(node_t *node, rpl_neighbor_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    int32 i, pos = -1;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        if (node->rpl_info->neighbor_list[i] == neighbor) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        if (neighbor->node != NULL)
            rs_error("node '%s' does not have node '%s' as a neighbor", node->phy_info->name, neighbor->node->phy_info->name);

        return FALSE;
    }

    rpl_neighbor_destroy(neighbor);

    for (i = pos; i < node->rpl_info->neighbor_count - 1; i++) {
        node->rpl_info->neighbor_list[i] = node->rpl_info->neighbor_list[i + 1];
    }

    node->rpl_info->neighbor_count--;
    node->rpl_info->neighbor_list = realloc(node->rpl_info->neighbor_list, node->rpl_info->neighbor_count * sizeof(rpl_neighbor_t *));
    if (node->rpl_info->neighbor_count == 0) {
        node->rpl_info->neighbor_list = NULL;
    }

    return TRUE;
}

void rpl_node_remove_all_neighbors(node_t *node)
{
    rs_assert(node != NULL);

    uint16 i;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        rpl_neighbor_t *neighbor = node->rpl_info->neighbor_list[i];

        rpl_neighbor_destroy(neighbor);
    }

    if (node->rpl_info->neighbor_list != NULL) {
        free(node->rpl_info->neighbor_list);
        node->rpl_info->neighbor_list = NULL;
    }

    node->rpl_info->neighbor_count = 0;
}

rpl_neighbor_t *rpl_node_find_neighbor_by_node(node_t *node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    int i;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        if (node->rpl_info->neighbor_list[i]->node == neighbor_node) {
            return node->rpl_info->neighbor_list[i];
        }
    }

    return NULL;
}

void rpl_node_add_parent(node_t *node, rpl_neighbor_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    dodag->parent_list = realloc(dodag->parent_list, (dodag->parent_count + 1) * sizeof(rpl_neighbor_t *));
    dodag->parent_list[dodag->parent_count++] = parent;
}

bool rpl_node_remove_parent(node_t *node, rpl_neighbor_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    int32 pos = -1, i;
    for (i = 0; i < dodag->parent_count; i++) {
        if (dodag->parent_list[i] == parent) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        if (parent->node != NULL)
            rs_error("node '%s' does not have node '%s' as a parent", node->phy_info->name, parent->node->phy_info->name);

        return FALSE;
    }

    for (i = pos; i < dodag->parent_count - 1; i++) {
        dodag->parent_list[i] = dodag->parent_list[i + 1];
    }

    dodag->parent_count--;
    dodag->parent_list = realloc(dodag->parent_list, dodag->parent_count * sizeof(rpl_neighbor_t *));
    if (dodag->parent_count == 0) {
        dodag->parent_list = NULL;
    }

    return TRUE;
}

void rpl_node_remove_all_parents(node_t *node)
{
    rs_assert(node != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    if (dodag->parent_list != NULL) {
        free(dodag->parent_list);
        dodag->parent_list = NULL;
    }

    dodag->parent_count = 0;
}

rpl_neighbor_t *rpl_node_find_parent_by_node(node_t *node, node_t *parent_node)
{
    rs_assert(node != NULL);
    rs_assert(parent_node != NULL);

    if (node->rpl_info->joined_dodag == NULL) {
        return NULL;
    }

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    int i;
    for (i = 0; i < dodag->parent_count; i++) {
        if (dodag->parent_list[i]->node == parent_node) {
            return dodag->parent_list[i];
        }
    }

    return NULL;
}

void rpl_node_add_sibling(node_t *node, rpl_neighbor_t *sibling)
{
    rs_assert(node != NULL);
    rs_assert(sibling != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    dodag->sibling_list = realloc(dodag->sibling_list, (dodag->sibling_count + 1) * sizeof(rpl_neighbor_t *));
    dodag->sibling_list[dodag->sibling_count++] = sibling;
}

bool rpl_node_remove_sibling(node_t *node, rpl_neighbor_t *sibling)
{
    rs_assert(node != NULL);
    rs_assert(sibling != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    int pos = -1, i;
    for (i = 0; i < dodag->sibling_count; i++) {
        if (dodag->sibling_list[i] == sibling) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        if (sibling->node != NULL)
            rs_error("node '%s' does not have node '%s' as a sibling", node->phy_info->name, sibling->node->phy_info->name);

        return FALSE;
    }

    for (i = pos; i < dodag->sibling_count - 1; i++) {
        dodag->sibling_list[i] = dodag->sibling_list[i + 1];
    }

    dodag->sibling_count--;
    dodag->sibling_list = realloc(dodag->sibling_list, dodag->sibling_count * sizeof(rpl_neighbor_t *));
    if (dodag->sibling_count == 0) {
        dodag->sibling_list = NULL;
    }

    return TRUE;
}

void rpl_node_remove_all_siblings(node_t *node)
{
    rs_assert(node != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    if (dodag->sibling_list != NULL) {
        free(dodag->sibling_list);
        dodag->sibling_list = NULL;
    }

    dodag->sibling_count = 0;
}

rpl_neighbor_t *rpl_node_find_sibling_by_node(node_t *node, node_t *sibling_node)
{
    rs_assert(node != NULL);
    rs_assert(sibling_node != NULL);

    if (node->rpl_info->joined_dodag == NULL) {
        return NULL;
    }

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    int i;
    for (i = 0; i < dodag->sibling_count; i++) {
        if (dodag->sibling_list[i]->node == sibling_node) {
            return dodag->sibling_list[i];
        }
    }

    return NULL;
}

node_t *rpl_node_get_next_hop(node_t *node, char *dst_address)
{
    return NULL;
}

bool rpl_send_dis(node_t *node, node_t *dst_node)
{
    rs_assert(node != NULL);

    return event_execute(rpl_event_id_after_dis_pdu_sent, node, dst_node, NULL);
}

bool rpl_receive_dis(node_t *node, node_t *src_node)
{
    rs_assert(node != NULL);

    bool all_ok = event_execute(rpl_event_id_after_dis_pdu_received, node, src_node, NULL);

    return all_ok;
}

bool rpl_send_dio(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu)
{
    rs_assert(node != NULL);

    return event_execute(rpl_event_id_after_dio_pdu_sent, node, dst_node, pdu);
}

bool rpl_receive_dio(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu)
{
    rs_assert(node != NULL);

    bool all_ok = event_execute(rpl_event_id_after_dio_pdu_received, node, src_node, pdu);

    rpl_dio_pdu_destroy(pdu);

    return all_ok;
}

bool rpl_send_dao(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu)
{
    rs_assert(node != NULL);

    return event_execute(rpl_event_id_after_dao_pdu_sent, node, dst_node, pdu);
}

bool rpl_receive_dao(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu)
{
    rs_assert(node != NULL);

    bool all_ok = event_execute(rpl_event_id_after_dao_pdu_received, node, src_node, pdu);

    rpl_dao_pdu_destroy(pdu);

    return all_ok;
}

bool rpl_event_after_node_wake(node_t *node)
{
    if (node->rpl_info->root_info->grounded) { /* if preconfigured as grounded root */
        if (rs_system->rpl_auto_sn_inc_interval > 0 ) {
            rs_system_schedule_event(node, rpl_event_id_after_seq_num_timer_timeout, NULL, NULL, rs_system->rpl_auto_sn_inc_interval);
        }

        start_as_root(node);
    }
    else { /* preconfigured as normal node */
        if (rs_system->rpl_start_silent) {
            start_as_root(node);
        }
        else {
            rpl_send_dis(node, NULL);
        }
    }

    return TRUE;
}

bool rpl_event_before_node_kill(node_t *node)
{
    /* put the node into "initial" state */
    if (node->rpl_info->root_info->dodag_id != NULL) {
        free(node->rpl_info->root_info->dodag_id);
        node->rpl_info->root_info->dodag_id = NULL;
    }

    if (node->rpl_info->joined_dodag != NULL) {
        rpl_dodag_destroy(node->rpl_info->joined_dodag);
        node->rpl_info->joined_dodag = NULL;
    }

    rpl_node_remove_all_neighbors(node);

    return TRUE;
}

bool rpl_event_after_dis_pdu_sent(node_t *node, node_t *dst_node)
{
    if (!icmp_send(node, dst_node, ICMP_TYPE_RPL, ICMP_RPL_CODE_DIS, NULL)) {
        rs_error("node '%s': failed to send ICMP message", node->phy_info->name);
        return FALSE;
    }

    return TRUE;
}

bool rpl_event_after_dis_pdu_received(node_t *node, node_t *src_node)
{
    rpl_dio_pdu_t *dio_pdu = create_current_dio_message(node, TRUE);

    if (dio_pdu != NULL) {
        rpl_send_dio(node, src_node, dio_pdu);
    }

    measure_connect_update_output();
    measure_sp_comp_update_output();
    measure_converg_update_output();

    return TRUE;
}

bool rpl_event_after_dio_pdu_sent(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu)
{
    if (!icmp_send(node, dst_node, ICMP_TYPE_RPL, ICMP_RPL_CODE_DIO, pdu)) {
        rs_error("node '%s': failed to send ICMP message", node->phy_info->name);
        return FALSE;
    }

    return TRUE;
}

bool rpl_event_after_dio_pdu_received(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu)
{
    rpl_neighbor_t *neighbor = rpl_node_find_neighbor_by_node(node, src_node);

    if (neighbor == NULL) { /* don't process DIO messages from unknown neighbors */
        return FALSE; /* this should never happen */
    }

    if (dio_pdu_changed(neighbor, pdu)) {
//        bool dodag_id_changed = dio_pdu_dodag_id_changed(neighbor, pdu);
//        bool dodag_seq_num_changed = dio_pdu_dodag_seq_num_changed(neighbor, pdu);
//        bool dodag_rank_increased = dio_pdu_dodag_rank_increased(neighbor, pdu);
        bool dodag_config_changed = dio_pdu_dodag_config_changed(neighbor, pdu);

        update_neighbor_dio_message(neighbor, pdu);

        bool same;
        rpl_dio_pdu_t *preferred_dodag_pdu = get_preferred_dodag_dio_pdu(node, &same);

        if (rpl_node_is_isolated(node)) {
            if (preferred_dodag_pdu != NULL) {
                join_dodag_iteration(node, preferred_dodag_pdu);
                rs_assert(choose_parents_and_siblings(node));
                reset_trickle_timer(node);
            }
        }
        else if (rpl_node_is_root(node)) {
            if (preferred_dodag_pdu != NULL && !same) {
                join_dodag_iteration(node, preferred_dodag_pdu);
                rs_assert(choose_parents_and_siblings(node));
            }
        }
        else if (rpl_node_is_poisoning(node)) {
            /* we ignore everything in this temporary state */
        }
        else if (rpl_node_is_joined(node)) {
            if (!same) {
                if (preferred_dodag_pdu != NULL) {
                    join_dodag_iteration(node, preferred_dodag_pdu);
                    rs_assert(choose_parents_and_siblings(node));
                }
                else {
                    start_as_root(node);
                }
            }
            else {
                if (dodag_config_changed) {
                    update_dodag_config(node, pdu);
                    reset_trickle_timer(node); // todo why doesn't this have an effect ??
                }
                rs_assert(choose_parents_and_siblings(node));
            }
        }
        else {
            return FALSE; /* this should never happen */
        }
    }

    measure_connect_update_output();
    measure_sp_comp_update_output();
    measure_converg_update_output();

    return TRUE;
}

bool rpl_event_after_dao_pdu_sent(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu)
{
    if (!icmp_send(node, dst_node, ICMP_TYPE_RPL, ICMP_RPL_CODE_DAO, pdu)) {
        rs_error("node '%s': failed to send ICMP message", node->phy_info->name);
        return FALSE;
    }

    return TRUE;
}

bool rpl_event_after_dao_pdu_received(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu)
{
    measure_connect_update_output();
    measure_sp_comp_update_output();
    measure_converg_update_output();

    return TRUE;
}

bool rpl_event_after_neighbor_attach(node_t *node, node_t *neighbor_node)
{
    rpl_node_add_neighbor(node, neighbor_node);

    return TRUE;
}

bool rpl_event_after_neighbor_detach(node_t *node, node_t *neighbor_node)
{
    // todo remove routes through this neighbor

    bool lost_pref_parent = FALSE;
    rpl_neighbor_t *parent = rpl_node_find_parent_by_node(node, neighbor_node);
    if (parent != NULL) { /* obviously we are joined, since we have parents */
        rpl_node_remove_parent(node, parent);

        if (rpl_node_is_pref_parent(node, parent)) {
            lost_pref_parent = TRUE;
        }
    }

    rpl_neighbor_t *sibling = rpl_node_find_sibling_by_node(node, neighbor_node);
    if (sibling != NULL) {
        rpl_node_remove_sibling(node, sibling);
    }

    rpl_neighbor_t *neighbor = rpl_node_find_neighbor_by_node(node, neighbor_node);
    if (neighbor != NULL) {
        rpl_node_remove_neighbor(node, neighbor);
    }
    else {
        return FALSE; /* this should never happen */
    }

    if (lost_pref_parent) {
        if (choose_parents_and_siblings(node)) { /* this could trigger floating or poisoning */
            reset_trickle_timer(node);
        }
    }

    return TRUE;
}

bool rpl_event_after_forward_failure(node_t *node)
{
    // todo measure failures number

    reset_trickle_timer(node);

    return TRUE;
}

bool rpl_event_after_forward_error(node_t *node)
{
    // todo measure errors number

    reset_trickle_timer(node);

    return TRUE;
}

bool rpl_event_after_trickle_timer_t_timeout(node_t *node)
{
    // todo implement DIO message redundancy detection

    if (rpl_node_is_root(node)) {
        if ((node->rpl_info->trickle_c >= node->rpl_info->root_info->dio_redundancy_constant) &&
                (node->rpl_info->root_info->dio_redundancy_constant != 0xFF)) {

            return TRUE;
        }
    }
    else if (rpl_node_is_joined(node)) {
        if ((node->rpl_info->trickle_c >= node->rpl_info->joined_dodag->dio_redundancy_constant) &&
                (node->rpl_info->joined_dodag->dio_redundancy_constant != 0xFF)) {

            return TRUE;
        }
    }
    else if (!rpl_node_is_poisoning(node)) {
        return FALSE; /* this should never happen */
    }

    rpl_dio_pdu_t *dio_pdu = create_current_dio_message(node, TRUE);
    if (dio_pdu != NULL)
        rpl_send_dio(node, NULL, dio_pdu);

    return TRUE;
}

bool rpl_event_after_trickle_timer_i_timeout(node_t *node)
{
    if (rpl_node_is_root(node)) {
        if (node->rpl_info->trickle_i_doublings_so_far < node->rpl_info->root_info->dio_interval_doublings) {
            node->rpl_info->trickle_i_doublings_so_far++;
            node->rpl_info->trickle_i *= 2;
        }
    }
    else if (rpl_node_is_joined(node)) {
        if (node->rpl_info->trickle_i_doublings_so_far < node->rpl_info->joined_dodag->dio_interval_doublings) {
            node->rpl_info->trickle_i_doublings_so_far++;
            node->rpl_info->trickle_i *= 2;
        }
    }
    else if (rpl_node_is_poisoning(node)) {
        if (node->rpl_info->poison_count_so_far < rs_system->rpl_poison_count) {
            node->rpl_info->poison_count_so_far++;
        }
        else { /* enough with poisoning, stop the trickle timer */
            bool same;
            rpl_dio_pdu_t *preferred_dodag_pdu = get_preferred_dodag_dio_pdu(node, &same);

            if (preferred_dodag_pdu != NULL && !same) {
                join_dodag_iteration(node, preferred_dodag_pdu);
                rs_assert(choose_parents_and_siblings(node));
                reset_trickle_timer(node);
            }
            else {
                start_as_root(node);
            }

            return TRUE;
        }
    }
    else {
        return FALSE; /* this should never happen */
    }

    // todo this should be "deterministic" random
    uint32 t = (rand() % (node->rpl_info->trickle_i / 2)) + node->rpl_info->trickle_i / 2;
    //uint32 t = 3 * node->rpl_info->trickle_i / 4;

    rs_system_schedule_event(node, rpl_event_id_after_trickle_timer_t_timeout, NULL, NULL, t);
    rs_system_schedule_event(node, rpl_event_id_after_trickle_timer_i_timeout, NULL, NULL, node->rpl_info->trickle_i);

    return TRUE;
}

bool rpl_event_after_seq_num_timer_timeout(node_t *node)
{
    if (!rpl_node_is_root(node)) { /* avoid incrementing the seq_num when we're not root (anymore) */
        return TRUE;
    }

    node->rpl_info->root_info->seq_num++;
    reset_trickle_timer(node);

    if (rs_system->rpl_auto_sn_inc_interval > 0) {
        rs_system_schedule_event(node, rpl_event_id_after_seq_num_timer_timeout, NULL, NULL, rs_system->rpl_auto_sn_inc_interval);
    }

    return TRUE;
}


    /**** local functions ****/

static rpl_neighbor_t *rpl_neighbor_create(node_t *node)
{
    rpl_neighbor_t *neighbor = malloc(sizeof(rpl_neighbor_t));

    neighbor->node = node;
    neighbor->is_dao_parent = FALSE;
    neighbor->last_dio_message = NULL;

    return neighbor;
}

static void rpl_neighbor_destroy(rpl_neighbor_t* neighbor)
{
    rs_assert(neighbor != NULL);

    if (neighbor->last_dio_message != NULL) {
        rpl_dio_pdu_destroy(neighbor->last_dio_message);
    }

    free(neighbor);
}

static rpl_root_info_t *rpl_root_info_create()
{
    rpl_root_info_t *root_info = malloc(sizeof(rpl_root_info_t));

    root_info->dodag_id = NULL;
    root_info->dodag_pref = RPL_DEFAULT_DAG_PREF;
    root_info->grounded = FALSE;
    root_info->dao_supported = rs_system->rpl_dao_supported;
    //root_info->dao_trigger = FALSE; todo make this a #define somewhere

    root_info->dio_interval_doublings = RPL_DEFAULT_DIO_INTERVAL_DOUBLINGS;
    root_info->dio_interval_min = RPL_DEFAULT_DIO_INTERVAL_MIN;
    root_info->dio_redundancy_constant = RPL_DEFAULT_DIO_REDUNDANCY_CONSTANT;

    root_info->max_rank_inc = RPL_DEFAULT_MAX_RANK_INC;
    root_info->min_hop_rank_inc = RPL_DEFAULT_MIN_HOP_RANK_INC;

    root_info->seq_num = 0;

    return root_info;
}

static void rpl_root_info_destroy(rpl_root_info_t *root_info)
{
    rs_assert(root_info != NULL);

    if (root_info->dodag_id != NULL) {
        free(root_info->dodag_id);
    }

    free(root_info);
}

static rpl_dodag_t *rpl_dodag_create(rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(dio_pdu != NULL);
    rs_assert(dio_pdu->dodag_config_suboption != NULL);

    rpl_dodag_t *dodag = malloc(sizeof(rpl_dodag_t));

    dodag->dodag_id = strdup(dio_pdu->dodag_id);
    dodag->dodag_pref = dio_pdu->dodag_pref;
    dodag->grounded = dio_pdu->grounded;
    dodag->dao_supported = dio_pdu->dao_supported;
    dodag->dao_trigger = dio_pdu->dao_trigger;

    dodag->dio_interval_doublings = dio_pdu->dodag_config_suboption->dio_interval_doublings;
    dodag->dio_interval_min = dio_pdu->dodag_config_suboption->dio_interval_min;
    dodag->dio_redundancy_constant = dio_pdu->dodag_config_suboption->dio_redundancy_constant;
    dodag->max_rank_inc = dio_pdu->dodag_config_suboption->max_rank_inc;
    dodag->min_hop_rank_inc = dio_pdu->dodag_config_suboption->min_hop_rank_inc;

    dodag->seq_num = dio_pdu->seq_num;
    dodag->lowest_rank = RPL_RANK_INFINITY;
    dodag->rank = RPL_RANK_INFINITY;

    dodag->parent_list = NULL;
    dodag->parent_count = 0;
    dodag->sibling_list = NULL;
    dodag->sibling_count = 0;
    dodag->pref_parent = NULL;

    return dodag;
}

static void rpl_dodag_destroy(rpl_dodag_t *dodag)
{
    rs_assert(dodag != NULL);

    if (dodag->dodag_id != NULL) {
        free(dodag->dodag_id);
    }

    if (dodag->parent_list != NULL) {
        free(dodag->parent_list);
    }

    if (dodag->sibling_list != NULL) {
        free(dodag->sibling_list);
    }

    free(dodag);
}

static bool dio_pdu_changed(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(neighbor != NULL);
    rs_assert(dio_pdu != NULL);

    if (neighbor->last_dio_message == NULL) {
        return TRUE;
    }
    else { /* neighbor->last_dio_message not NULL */
        bool base_changed = (strcmp(neighbor->last_dio_message->dodag_id, dio_pdu->dodag_id) != 0) ||
                (neighbor->last_dio_message->dodag_pref != dio_pdu->dodag_pref) ||
                (neighbor->last_dio_message->seq_num != dio_pdu->seq_num) ||
                (neighbor->last_dio_message->rank != dio_pdu->rank) ||
                (neighbor->last_dio_message->dstn != dio_pdu->dstn) ||
                (neighbor->last_dio_message->dao_stored != dio_pdu->dao_stored) ||
                (neighbor->last_dio_message->grounded != dio_pdu->grounded) ||
                (neighbor->last_dio_message->dao_supported != dio_pdu->dao_supported) ||
                (neighbor->last_dio_message->dao_trigger != dio_pdu->dao_trigger);

        bool dodag_config_changed = dio_pdu_dodag_config_changed(neighbor, dio_pdu);

        return base_changed || dodag_config_changed;
    }
}

static bool dio_pdu_dodag_id_changed(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(neighbor != NULL);
    rs_assert(dio_pdu != NULL);

    if (neighbor->last_dio_message == NULL) {
        return TRUE;
    }
    else {
        return (strcmp(neighbor->last_dio_message->dodag_id, dio_pdu->dodag_id) != 0);
    }
}

static bool dio_pdu_dodag_seq_num_changed(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(neighbor != NULL);
    rs_assert(dio_pdu != NULL);

    if (neighbor->last_dio_message == NULL) {
        return TRUE;
    }
    else {
        return (neighbor->last_dio_message->seq_num != dio_pdu->seq_num);
    }
}

static bool dio_pdu_dodag_rank_increased(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(neighbor != NULL);
    rs_assert(dio_pdu != NULL);

    if (neighbor->last_dio_message == NULL) {
        return TRUE;
    }
    else {
        // todo use compare_ranks()
        return (neighbor->last_dio_message->rank > dio_pdu->rank);
    }
}

static bool dio_pdu_dodag_config_changed(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(neighbor != NULL);
    rs_assert(dio_pdu != NULL);

    if (dio_pdu->dodag_config_suboption == NULL) {
        return FALSE;
    }

    if (neighbor->last_dio_message == NULL || neighbor->last_dio_message->dodag_config_suboption == NULL) {
        return TRUE;
    }
    else {
        return (neighbor->last_dio_message->dodag_config_suboption->dio_interval_doublings != dio_pdu->dodag_config_suboption->dio_interval_doublings) ||
                (neighbor->last_dio_message->dodag_config_suboption->dio_interval_min != dio_pdu->dodag_config_suboption->dio_interval_min) ||
                (neighbor->last_dio_message->dodag_config_suboption->dio_redundancy_constant != dio_pdu->dodag_config_suboption->dio_redundancy_constant) ||
                (neighbor->last_dio_message->dodag_config_suboption->max_rank_inc != dio_pdu->dodag_config_suboption->max_rank_inc) ||
                (neighbor->last_dio_message->dodag_config_suboption->min_hop_rank_inc != dio_pdu->dodag_config_suboption->min_hop_rank_inc);
    }
}

static void start_dio_poisoning(node_t *node)
{
    rs_assert(node != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);

    rpl_node_remove_all_parents(node);
    rpl_node_remove_all_siblings(node);
    node->rpl_info->joined_dodag->pref_parent = NULL;

    node->rpl_info->joined_dodag->rank = RPL_RANK_INFINITY;
    node->rpl_info->joined_dodag->lowest_rank = RPL_RANK_INFINITY;
    node->rpl_info->poison_count_so_far = 0;

    reset_trickle_timer(node);
}

static void start_as_root(node_t *node)
{
    rs_assert(node != NULL);

    if (node->rpl_info->joined_dodag != NULL) {
        rpl_dodag_destroy(node->rpl_info->joined_dodag);
        node->rpl_info->joined_dodag = NULL;
    }

    node->rpl_info->root_info->dodag_id = strdup(node->ip_info->address);
    node->rpl_info->root_info->seq_num = 0;

    reset_trickle_timer(node);
}

static void join_dodag_iteration(node_t *node, rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(node != NULL);
    rs_assert(dio_pdu != NULL);

    if (node->rpl_info->joined_dodag != NULL) {
        rpl_dodag_destroy(node->rpl_info->joined_dodag);
    }

    if (node->rpl_info->root_info->dodag_id != NULL) { /* if we were previously a root */
        free(node->rpl_info->root_info->dodag_id);
        node->rpl_info->root_info->dodag_id = NULL;
    }

    node->rpl_info->joined_dodag = rpl_dodag_create(dio_pdu);
}

static void update_dodag_config(node_t *node, rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(node != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);
    rs_assert(dio_pdu != NULL);
    rs_assert(dio_pdu->dodag_config_suboption != NULL);

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    dodag->dio_interval_doublings = dio_pdu->dodag_config_suboption->dio_interval_doublings;
    dodag->dio_interval_min = dio_pdu->dodag_config_suboption->dio_interval_min;
    dodag->dio_redundancy_constant = dio_pdu->dodag_config_suboption->dio_redundancy_constant;
    dodag->max_rank_inc = dio_pdu->dodag_config_suboption->max_rank_inc;
    dodag->min_hop_rank_inc = dio_pdu->dodag_config_suboption->min_hop_rank_inc;
}

static bool choose_parents_and_siblings(node_t *node)
{
    rs_assert(node != NULL);
    rs_assert(node->rpl_info->joined_dodag != NULL);

    rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

    rpl_node_remove_all_parents(node);
    rpl_node_remove_all_siblings(node);
    dodag->pref_parent = NULL;

    int16 best_rank_index = -1;

    uint8 matching_ranks[node->rpl_info->neighbor_count];

    uint16 i;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        rpl_neighbor_t *neighbor = node->rpl_info->neighbor_list[i];

        matching_ranks[i] = RPL_RANK_INFINITY;

        if (neighbor->last_dio_message == NULL) { /* ignore neighbors who haven't sent any DIO */
            continue;
        }

        if (strcmp(neighbor->last_dio_message->dodag_id, dodag->dodag_id) != 0) { /* ignore neighbors from different DODAGs */
            continue;
        }

        if (neighbor->last_dio_message->seq_num != dodag->seq_num) { /* ignore neighbors from different DODAG iterations */
            continue;
        }

        matching_ranks[i] = compute_candidate_rank(node, neighbor);

        if (best_rank_index == -1) {
            best_rank_index = i;
            continue;
        }

        if (matching_ranks[i] < matching_ranks[best_rank_index]) {
            best_rank_index = i;
        }
    }

    // todo use compare_ranks()
    if (best_rank_index == -1) { /* no valid neighbors found for this current DODAG iteration */
        bool same;
        rpl_dio_pdu_t *preferred_dodag_pdu = get_preferred_dodag_dio_pdu(node, &same);

        if (preferred_dodag_pdu != NULL && !same) {
            join_dodag_iteration(node, preferred_dodag_pdu);
            rs_assert(choose_parents_and_siblings(node));
        }
        else {
            start_as_root(node);
        }

        return FALSE;
    }

    uint8 best_rank = matching_ranks[best_rank_index];
    if (best_rank - dodag->lowest_rank > dodag->max_rank_inc ) { /* rank would increase too much */
        start_dio_poisoning(node);

        return FALSE;
    }

    dodag->rank = best_rank;
    if (best_rank < dodag->lowest_rank) {
        dodag->lowest_rank = best_rank;
    }

    dodag->pref_parent = node->rpl_info->neighbor_list[best_rank_index];

    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        if (matching_ranks[i] == RPL_RANK_INFINITY) {
            continue;
        }

        rpl_neighbor_t *neighbor = node->rpl_info->neighbor_list[i];

        if (neighbor->last_dio_message->rank < best_rank) { /* a parent */
            rpl_node_add_parent(node, neighbor);
        }
        else if (neighbor->last_dio_message->rank == best_rank) { /* a sibling */
            rpl_node_add_sibling(node, neighbor);
        }
    }

    return TRUE;
}

static rpl_dio_pdu_t *get_preferred_dodag_dio_pdu(node_t *node, bool *same)
{
    rs_assert(node != NULL);
    rs_assert(same != NULL);

    rpl_dio_pdu_t *best_dio_pdu = NULL;
    rpl_dio_pdu_t *fake_dio_pdu = NULL;

    /* a node has its own passive DODAG iteration, coming from the default root configuration,
     * which should be taken into consideration as well
     * thus we create a fake DIO message corresponding to this own DODAG iteration */

    // todo eliminate this workaround foring the root-based DIO message to be created

    rpl_dodag_t *temp_dodag = node->rpl_info->joined_dodag;
    node->rpl_info->joined_dodag = NULL;
    node->rpl_info->root_info->dodag_id = strdup(node->ip_info->address);

    fake_dio_pdu = create_current_dio_message(node, TRUE);
    best_dio_pdu = fake_dio_pdu;

    node->rpl_info->joined_dodag = temp_dodag;

    uint16 i;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        rpl_neighbor_t *neighbor = node->rpl_info->neighbor_list[i];

        if (neighbor->last_dio_message == NULL) { /* ignore neighbors who haven't sent any DIO */
            continue;
        }

        if (neighbor->last_dio_message->dodag_config_suboption == NULL) { /* ignore neighbors for whom no dodag config info is available */
            continue;
        }

        if (best_dio_pdu == NULL) { /* the very first one */
            best_dio_pdu = neighbor->last_dio_message;
            continue;
        }

        if (!best_dio_pdu->grounded && neighbor->last_dio_message->grounded) {
            best_dio_pdu = neighbor->last_dio_message;
        }
        else if (best_dio_pdu->grounded == neighbor->last_dio_message->grounded) {
            if (best_dio_pdu->dodag_pref < neighbor->last_dio_message->dodag_pref) {
                best_dio_pdu = neighbor->last_dio_message;
            }
            else if (best_dio_pdu->dodag_pref == neighbor->last_dio_message->dodag_pref) {
                if (best_dio_pdu->seq_num < neighbor->last_dio_message->seq_num) {
                    best_dio_pdu = neighbor->last_dio_message;
                }
            }
        }
    }

    if (best_dio_pdu != NULL) {
        if (rpl_node_is_root(node)) {
            *same = (strcmp(node->rpl_info->root_info->dodag_id, best_dio_pdu->dodag_id) == 0) &&
                    (node->rpl_info->root_info->seq_num == best_dio_pdu->seq_num);
        }
        else if (rpl_node_is_joined(node) || rpl_node_is_poisoning(node)) {
            rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

            *same = (dodag != NULL) &&
                    (strcmp(dodag->dodag_id, best_dio_pdu->dodag_id) == 0) &&
                    (dodag->seq_num == best_dio_pdu->seq_num);
        }
        else {
            *same = FALSE;
        }
    }
    else {
        if (rpl_node_is_root(node)) {
            *same = TRUE;
        }
        else if (rpl_node_is_joined(node)) {
            *same = FALSE;
        }
        else {
            *same = FALSE;
        }
    }

    /* destroy a previously created fake DIO message */
    if (fake_dio_pdu != NULL) {
        rpl_dio_pdu_destroy(fake_dio_pdu);
    }

    return best_dio_pdu;
}

static void reset_trickle_timer(node_t *node)
{
    rs_assert(node != NULL);

    if (rpl_node_is_root(node)) {
        node->rpl_info->trickle_i = pow(2, node->rpl_info->root_info->dio_interval_min);
    }
    else {
        node->rpl_info->trickle_i = pow(2, node->rpl_info->joined_dodag->dio_interval_min);
    }

    node->rpl_info->trickle_i_doublings_so_far = 0;
    node->rpl_info->trickle_c = 0;

    // todo this should use a "deterministic" random
    uint32 t = (rand() % (node->rpl_info->trickle_i / 2)) + node->rpl_info->trickle_i / 2;
    //uint32 t = 3 * node->rpl_info->trickle_i / 4;

    rs_system_cancel_event(node, rpl_event_id_after_trickle_timer_t_timeout, NULL, NULL, 0);
    rs_system_cancel_event(node, rpl_event_id_after_trickle_timer_i_timeout, NULL, NULL, 0);

    rs_system_schedule_event(node, rpl_event_id_after_trickle_timer_t_timeout, NULL, NULL, t);
    rs_system_schedule_event(node, rpl_event_id_after_trickle_timer_i_timeout, NULL, NULL, node->rpl_info->trickle_i);
}

static rpl_dio_pdu_t *create_current_dio_message(node_t *node, bool include_dodag_config)
{
    // todo from time to time avoid to include dodag config

    rs_assert(node != NULL);

    rpl_dio_pdu_t *dio_pdu = NULL;

    if (rpl_node_is_joined(node)) { /* joined node */
        rpl_dodag_t *dodag = node->rpl_info->joined_dodag;
        dio_pdu = rpl_dio_pdu_create();

        dio_pdu->dodag_id = strdup(dodag->dodag_id);
        dio_pdu->dodag_pref = dodag->dodag_pref;
        dio_pdu->seq_num = dodag->seq_num;

        dio_pdu->rank = dodag->rank;
        dio_pdu->dstn = 0;
        dio_pdu->dao_stored = FALSE;

        dio_pdu->grounded = dodag->grounded;
        dio_pdu->dao_supported = dodag->dao_supported;
        dio_pdu->dao_trigger = dodag->dao_trigger;

        if (include_dodag_config) {
            dio_pdu->dodag_config_suboption = rpl_dio_suboption_dodag_config_create();

            dio_pdu->dodag_config_suboption->dio_interval_doublings = dodag->dio_interval_doublings;
            dio_pdu->dodag_config_suboption->dio_interval_min = dodag->dio_interval_min;
            dio_pdu->dodag_config_suboption->dio_redundancy_constant = dodag->dio_redundancy_constant;
            dio_pdu->dodag_config_suboption->max_rank_inc = dodag->max_rank_inc;
            dio_pdu->dodag_config_suboption->min_hop_rank_inc = dodag->min_hop_rank_inc;
        }
    }
    else if (rpl_node_is_root(node)) { /* root node */
        rpl_root_info_t *root_info = node->rpl_info->root_info;
        dio_pdu = rpl_dio_pdu_create();

        dio_pdu->dodag_id = strdup(root_info->dodag_id);
        dio_pdu->dodag_pref = root_info->dodag_pref;
        dio_pdu->seq_num = root_info->seq_num;

        dio_pdu->rank = RPL_RANK_ROOT;
        dio_pdu->dstn = 0;
        dio_pdu->dao_stored = FALSE;

        dio_pdu->grounded = root_info->grounded;
        dio_pdu->dao_supported = root_info->dao_supported;
        dio_pdu->dao_trigger = root_info->dao_trigger;

        if (include_dodag_config) {
            dio_pdu->dodag_config_suboption = rpl_dio_suboption_dodag_config_create();

            dio_pdu->dodag_config_suboption->dio_interval_doublings = root_info->dio_interval_doublings;
            dio_pdu->dodag_config_suboption->dio_interval_min = root_info->dio_interval_min;
            dio_pdu->dodag_config_suboption->dio_redundancy_constant = root_info->dio_redundancy_constant;
            dio_pdu->dodag_config_suboption->max_rank_inc = root_info->max_rank_inc;
            dio_pdu->dodag_config_suboption->min_hop_rank_inc = root_info->min_hop_rank_inc;
        }
    }
    else {
        dio_pdu = NULL;
    }

    return dio_pdu;
}

static void update_neighbor_dio_message(rpl_neighbor_t *neighbor, rpl_dio_pdu_t *dio_pdu)
{
    rs_assert(neighbor != NULL);
    rs_assert(dio_pdu != NULL);

    if (neighbor->last_dio_message == NULL) {
        neighbor->last_dio_message = rpl_dio_pdu_duplicate(dio_pdu);
    }
    else {
        if (neighbor->last_dio_message->dodag_id != NULL)
            free(neighbor->last_dio_message->dodag_id);

        neighbor->last_dio_message->dodag_id = strdup(dio_pdu->dodag_id);
        neighbor->last_dio_message->dodag_pref = dio_pdu->dodag_pref;
        neighbor->last_dio_message->seq_num = dio_pdu->seq_num;

        neighbor->last_dio_message->rank = dio_pdu->rank;
        neighbor->last_dio_message->dstn = dio_pdu->dstn;
        neighbor->last_dio_message->dao_stored = dio_pdu->dao_stored;

        neighbor->last_dio_message->grounded = dio_pdu->grounded;
        neighbor->last_dio_message->dao_supported = dio_pdu->dao_supported;
        neighbor->last_dio_message->dao_trigger = dio_pdu->dao_trigger;

        if (dio_pdu->dodag_config_suboption != NULL) {
            if (neighbor->last_dio_message->dodag_config_suboption == NULL) {
                neighbor->last_dio_message->dodag_config_suboption = rpl_dio_suboption_dodag_config_create();
            }

            neighbor->last_dio_message->dodag_config_suboption->dio_interval_min = dio_pdu->dodag_config_suboption->dio_interval_min;
            neighbor->last_dio_message->dodag_config_suboption->dio_interval_doublings = dio_pdu->dodag_config_suboption->dio_interval_doublings;
            neighbor->last_dio_message->dodag_config_suboption->dio_redundancy_constant = dio_pdu->dodag_config_suboption->dio_redundancy_constant;
            neighbor->last_dio_message->dodag_config_suboption->min_hop_rank_inc = dio_pdu->dodag_config_suboption->min_hop_rank_inc;
            neighbor->last_dio_message->dodag_config_suboption->max_rank_inc = dio_pdu->dodag_config_suboption->max_rank_inc;
        }
    }
}

static int8 compare_ranks(uint8 rank1, uint8 rank2, uint8 min_hop_rank_inc)
{
    if (rank1 / min_hop_rank_inc > rank2 / min_hop_rank_inc) {
        return 1;
    }
    else if (rank1 / min_hop_rank_inc == rank2 / min_hop_rank_inc) {
        return 0;
    }
    else {
        return -1;
    }
}

static uint8 compute_candidate_rank(node_t *node, rpl_neighbor_t *neighbor)
{
    percent_t send_link_quality = rs_system_get_link_quality(node, neighbor->node);
    percent_t receive_link_quality = rs_system_get_link_quality(neighbor->node, node);
    percent_t link_quality = (send_link_quality + receive_link_quality) / 2;

    uint8 rank = (RPL_MAXIMUM_RANK_INCREMENT - RPL_MINIMUM_RANK_INCREMENT) * pow((1 - link_quality), 2) + RPL_MINIMUM_RANK_INCREMENT;

    return neighbor->last_dio_message->rank + rank;
}

static void event_arg_str_one_node_func(void *data1, void *data2, char *str1, char *str2, uint16 len)
{
    node_t *node = data1;

    snprintf(str1, len, "%s", (node != NULL ? node->phy_info->name : "broadcast"));
    str2[0] = '\0';
}
