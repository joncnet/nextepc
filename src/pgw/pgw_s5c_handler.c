#include "gtp/gtp_types.h"
#include "gtp/gtp_node.h"
#include "gtp/gtp_path.h"

#include "app/context.h"
#include "pgw_event.h"
#include "pgw_context.h"
#include "pgw_gtp_path.h"
#include "pgw_s5c_handler.h"

void pgw_s5c_handle_create_session_request(
        pgw_sess_t *sess, gtp_xact_t *xact, gtp_create_session_request_t *req)
{
    int rv;
    gtp_f_teid_t *sgw_s5c_teid, *sgw_s5u_teid;
    gtp_node_t *sgw = NULL;
    pgw_bearer_t *bearer = NULL;
    gtp_bearer_qos_t bearer_qos;
    gtp_ambr_t *ambr = NULL;
    gtp_uli_t uli;
    uint16_t decoded = 0;

    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(req);
    bearer = pgw_default_bearer_in_sess(sess);
    ogs_assert(bearer);

    ogs_debug("[PGW] Create Session Reqeust");
    if (req->imsi.presence == 0)
    {
        ogs_error("No IMSI");
        return;
    }
    if (req->sender_f_teid_for_control_plane.presence == 0)
    {
        ogs_error("No TEID");
        return;
    }
    if (req->bearer_contexts_to_be_created.presence == 0)
    {
        ogs_error("No Bearer");
        return;
    }
    if (req->bearer_contexts_to_be_created.bearer_level_qos.presence == 0)
    {
        ogs_error("No EPS Bearer QoS");
        return;
    }
    if (req->bearer_contexts_to_be_created.s5_s8_u_sgw_f_teid.presence == 0)
    {
        ogs_error("No TEID");
        return;
    }
    if (req->user_location_information.presence == 0)
    {
        ogs_error("No User Location Inforamtion");
        return;
    }
    
    /* Set IMSI */
    sess->imsi_len = req->imsi.len;
    memcpy(sess->imsi, req->imsi.data, sess->imsi_len);
    ogs_buffer_to_bcd(sess->imsi, sess->imsi_len, sess->imsi_bcd);

    /* Control Plane(DL) : SGW-S5C */
    sgw_s5c_teid = req->sender_f_teid_for_control_plane.data;
    ogs_assert(sgw_s5c_teid);
    sess->sgw_s5c_teid = ntohl(sgw_s5c_teid->teid);

    /* Control Plane(DL) : SGW-S5U */
    sgw_s5u_teid = req->bearer_contexts_to_be_created.s5_s8_u_sgw_f_teid.data;
    ogs_assert(sgw_s5u_teid);
    bearer->sgw_s5u_teid = ntohl(sgw_s5u_teid->teid);

    ogs_debug("    SGW_S5C_TEID[0x%x] PGW_S5C_TEID[0x%x]",
            sess->sgw_s5c_teid, sess->pgw_s5c_teid);
    ogs_debug("    SGW_S5U_TEID[%d] PGW_S5U_TEID[%d]",
            bearer->sgw_s5u_teid, bearer->pgw_s5u_teid);

    sgw = gtp_node_find(&pgw_self()->sgw_s5u_list, sgw_s5u_teid);
    if (!sgw)
    {
        sgw = gtp_node_add(&pgw_self()->sgw_s5u_list, sgw_s5u_teid,
            pgw_self()->gtpu_port,
            context_self()->config.parameter.no_ipv4,
            context_self()->config.parameter.no_ipv6,
            context_self()->config.parameter.prefer_ipv4);
        ogs_assert(sgw);

        rv = gtp_connect(
                pgw_self()->gtpu_sock, pgw_self()->gtpu_sock6, sgw);
        ogs_assert(rv == OGS_OK);
    }
    /* Setup GTP Node */
    SETUP_GTP_NODE(bearer, sgw);

    decoded = gtp_parse_bearer_qos(&bearer_qos,
        &req->bearer_contexts_to_be_created.bearer_level_qos);
    ogs_assert(req->bearer_contexts_to_be_created.bearer_level_qos.len ==
            decoded);
    sess->pdn.qos.qci = bearer_qos.qci;
    sess->pdn.qos.arp.priority_level = bearer_qos.priority_level;
    sess->pdn.qos.arp.pre_emption_capability =
                    bearer_qos.pre_emption_capability;
    sess->pdn.qos.arp.pre_emption_vulnerability =
                    bearer_qos.pre_emption_vulnerability;

    /* Set AMBR if available */
    if (req->aggregate_maximum_bit_rate.presence)
    {
        ambr = req->aggregate_maximum_bit_rate.data;
        sess->pdn.ambr.downlink = ntohl(ambr->downlink);
        sess->pdn.ambr.uplink = ntohl(ambr->uplink);
    }
    
    /* Set User Location Information */
    decoded = gtp_parse_uli(&uli, &req->user_location_information);
    ogs_assert(req->user_location_information.len == decoded);
    memcpy(&sess->tai.plmn_id, &uli.tai.plmn_id, sizeof(uli.tai.plmn_id));
    sess->tai.tac = uli.tai.tac;
    memcpy(&sess->e_cgi.plmn_id, &uli.e_cgi.plmn_id, sizeof(uli.e_cgi.plmn_id));
    sess->e_cgi.cell_id = uli.e_cgi.cell_id;
}

void pgw_s5c_handle_delete_session_request(
        pgw_sess_t *sess, gtp_xact_t *xact, gtp_delete_session_request_t *req)
{
    ogs_assert(sess);

    ogs_debug("[PGW] Delete Session Request");
    ogs_debug("    SGW_S5C_TEID[0x%x] PGW_S5C_TEID[0x%x]",
            sess->sgw_s5c_teid, sess->pgw_s5c_teid);
}

void pgw_s5c_handle_create_bearer_response(
        pgw_sess_t *sess, gtp_xact_t *xact, gtp_create_bearer_response_t *req)
{
    int rv;
    gtp_f_teid_t *sgw_s5u_teid, *pgw_s5u_teid;
    gtp_node_t *sgw = NULL;
    pgw_bearer_t *bearer = NULL;

    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(req);

    ogs_debug("[PGW] Create Bearer Response");
    ogs_debug("    SGW_S5C_TEID[0x%x] PGW_S5C_TEID[0x%x]",
            sess->sgw_s5c_teid, sess->pgw_s5c_teid);
    if (req->bearer_contexts.presence == 0)
    {
        ogs_error("No Bearer");
        return;
    }
    if (req->bearer_contexts.eps_bearer_id.presence == 0)
    {
        ogs_error("No EPS Bearer ID");
        return;
    }
    if (req->bearer_contexts.s5_s8_u_pgw_f_teid.presence == 0)
    {
        ogs_error("No PGW TEID");
        return;
    }
    if (req->bearer_contexts.s5_s8_u_sgw_f_teid.presence == 0)
    {
        ogs_error("No SGW TEID");
        return;
    }

    /* Correlate with PGW-S%U-TEID */
    pgw_s5u_teid = req->bearer_contexts.s5_s8_u_pgw_f_teid.data;
    ogs_assert(pgw_s5u_teid);

    /* Find the Bearer by PGW-S5U-TEID */
    bearer = pgw_bearer_find_by_pgw_s5u_teid(ntohl(pgw_s5u_teid->teid));
    ogs_assert(bearer);

    /* Set EBI */
    bearer->ebi = req->bearer_contexts.eps_bearer_id.u8;

    /* Data Plane(DL) : SGW-S5U */
    sgw_s5u_teid = req->bearer_contexts.s5_s8_u_sgw_f_teid.data;
    bearer->sgw_s5u_teid = ntohl(sgw_s5u_teid->teid);
    sgw = gtp_node_find(&pgw_self()->sgw_s5u_list, sgw_s5u_teid);
    if (!sgw)
    {
        sgw = gtp_node_add(&pgw_self()->sgw_s5u_list, sgw_s5u_teid,
            pgw_self()->gtpu_port,
            context_self()->config.parameter.no_ipv4,
            context_self()->config.parameter.no_ipv6,
            context_self()->config.parameter.prefer_ipv4);
        ogs_assert(sgw);

        rv = gtp_connect(pgw_self()->gtpu_sock, pgw_self()->gtpu_sock6, sgw);
        ogs_assert(rv == OGS_OK);
    }
    /* Setup GTP Node */
    SETUP_GTP_NODE(bearer, sgw);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);
    
    ogs_debug("[PGW] Create Bearer Response : SGW[0x%x] --> PGW[0x%x]",
            sess->sgw_s5c_teid, sess->pgw_s5c_teid);
}

void pgw_s5c_handle_update_bearer_response(
        pgw_sess_t *sess, gtp_xact_t *xact, gtp_update_bearer_response_t *req)
{
    int rv;

    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(req);

    ogs_debug("[PGW] Update Bearer Request");
    ogs_debug("    SGW_S5C_TEID[0x%x] PGW_S5C_TEID[0x%x]",
            sess->sgw_s5c_teid, sess->pgw_s5c_teid);
    if (req->bearer_contexts.presence == 0)
    {
        ogs_error("No Bearer");
        return;
    }
    if (req->bearer_contexts.eps_bearer_id.presence == 0)
    {
        ogs_error("No EPS Bearer ID");
        return;
    }

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);
    
    ogs_debug("[PGW] Update Bearer Response : SGW[0x%x] --> PGW[0x%x]",
            sess->sgw_s5c_teid, sess->pgw_s5c_teid);
}

void pgw_s5c_handle_delete_bearer_response(
        pgw_sess_t *sess, gtp_xact_t *xact, gtp_delete_bearer_response_t *req)
{
    int rv;
    pgw_bearer_t *bearer = NULL;

    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(req);

    ogs_debug("[PGW] Delete Bearer Request");
    ogs_debug("    SGW_S5C_TEID[0x%x] PGW_S5C_TEID[0x%x]",
            sess->sgw_s5c_teid, sess->pgw_s5c_teid);
    if (req->bearer_contexts.presence == 0)
    {
        ogs_error("No Bearer");
        return;
    }
    if (req->bearer_contexts.eps_bearer_id.presence == 0)
    {
        ogs_error("No EPS Bearer ID");
        return;
    }

    bearer = pgw_bearer_find_by_ebi(
            sess, req->bearer_contexts.eps_bearer_id.u8);
    ogs_assert(bearer);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);
    
    ogs_debug("[PGW] Delete Bearer Response : SGW[0x%x] --> PGW[0x%x]",
            sess->sgw_s5c_teid, sess->pgw_s5c_teid);

    pgw_bearer_remove(bearer);
}
