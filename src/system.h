
#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "base.h"
#include "node.h"

#include "proto/phy.h"
#include "proto/mac.h"
#include "proto/ip.h"
#include "proto/rpl.h"

#define DEFAULT_NO_LINK_DIST_THRESH     10
#define DEFAULT_PHY_TRANSMIT_MODE       PHY_TRANSMIT_MODE_BLOCK


typedef struct rs_system_t {

    /* params */
    coord_t             no_link_dist_thresh;
    uint8               phy_transmit_mode;

    coord_t             width;
    coord_t             height;

    /* nodes */
    node_t **           node_list;
    uint16              node_count;

    /* mutexes */
    GMutex *            nodes_mutex;
    GMutex *            params_mutex;

} rs_system_t;

extern rs_system_t *rs_system;


bool                        rs_system_create();
bool                        rs_system_destroy();

    /* params */
coord_t                     rs_system_get_no_link_dist_thresh();
void                        rs_system_set_no_link_dist_thresh(coord_t thresh);

uint8                       rs_system_get_transmit_mode();
void                        rs_system_set_transmit_mode(uint8 mode);

coord_t                     rs_system_get_width();
coord_t                     rs_system_get_height();
void                        rs_system_set_width(coord_t width);
void                        rs_system_set_height(coord_t height);

    /* nodes */
bool                        rs_system_add_node(node_t *node);
bool                        rs_system_remove_node(node_t *node);
node_t *                    rs_system_find_node_by_name(char *name);
node_t **                   rs_system_get_nodes(uint16 *node_count);

percent_t                   rs_system_get_link_quality(node_t *src_node, node_t *dst_node);

bool                        rs_system_send_rpl_dis(node_t *src_node, node_t *dst_node);
bool                        rs_system_send_rpl_dio(node_t *src_node, node_t *dst_node, rpl_dio_pdu_t *pdu);
bool                        rs_system_send_rpl_dao(node_t *src_node, node_t *dst_node, rpl_dao_pdu_t *pdu);

bool                        rs_system_process_message(node_t *node, phy_pdu_t *message);


#endif /* SYSTEM_H_ */