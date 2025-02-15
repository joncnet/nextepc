#include "mme_context.h"
#include "s1ap-path.h"
#include "s1ap_build.h"
#include "sbc_handler.h"

void sbc_handle_write_replace_warning_request(sbc_pws_data_t *sbc_pws)
{
    ogs_pkbuf_t *s1apbuf = NULL;
    ogs_hash_index_t *hi = NULL;
    mme_enb_t *enb = NULL;
    int i, j, flag;
    int rv;

    /* Find enB with matched TAI */
    for (hi = mme_enb_first(); hi; hi = mme_enb_next(hi))
    {
        flag = 0;
        enb = mme_enb_this(hi);
        if (sbc_pws->no_of_tai > 0)
        {
            for (i = 0, flag = 0; i < enb->num_of_supported_ta_list; i++)
            {
                for (j = 0; j < sbc_pws->no_of_tai; j++)
                {
                    if (!memcmp(&enb->supported_ta_list[i],
                                &sbc_pws->tai[j], sizeof(tai_t)))
                        flag = 1;

                    if (flag) break;
                }
                if (flag) break;
            }
        }
        else
            flag = 1;

        if (flag)
        {
            s1apbuf = NULL;

            /* Buidl S1AP Write Replace Warning Request message */
            rv = s1ap_build_write_replace_warning_request(&s1apbuf, sbc_pws);
            ogs_assert(rv == OGS_OK);

            /* Send to enb */
            ogs_assert(s1ap_send_to_enb(
                    enb, s1apbuf, S1AP_NON_UE_SIGNALLING) == OGS_OK);
        }
    }
}

void sbc_handle_stop_warning_request(sbc_pws_data_t *sbc_pws)
{
    ogs_pkbuf_t *s1apbuf = NULL;
    ogs_hash_index_t *hi = NULL;
    mme_enb_t *enb = NULL;
    int i, j, flag;
    int rv;

    /* Find enB with matched TAI */
    for (hi = mme_enb_first(); hi; hi = mme_enb_next(hi))
    {
        flag = 0;
        enb = mme_enb_this(hi);
        if (sbc_pws->no_of_tai > 0)
        {
            for (i = 0, flag = 0; i < enb->num_of_supported_ta_list; i++)
            {
                for (j = 0; j < sbc_pws->no_of_tai; j++)
                {
                    if (!memcmp(&enb->supported_ta_list[i],
                                &sbc_pws->tai[j], sizeof(tai_t)))
                        flag = 1;

                    if (flag) break;
                }
                if (flag) break;
            }
        }
        else
            flag = 1;

        if (flag)
        {
            s1apbuf = NULL;

            /* Buidl S1AP Kill request message */
            rv = s1ap_build_kill_request(&s1apbuf, sbc_pws);
            ogs_assert(rv == OGS_OK && s1apbuf);

            /* Send to enb */
            ogs_assert(s1ap_send_to_enb(
                    enb, s1apbuf, S1AP_NON_UE_SIGNALLING) == OGS_OK);
        }
    }
}
