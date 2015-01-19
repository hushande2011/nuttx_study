/****************************************************************************
 * net/udp/udp_send.c
 *
 *   Copyright (C) 2007-2009, 2011, 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for NuttX from logic in uIP which also has a BSD-like license:
 *
 *   Original author Adam Dunkels <adam@dunkels.com>
 *   Copyright () 2001-2003, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#if defined(CONFIG_NET) && defined(CONFIG_NET_UDP)

#include <string.h>
#include <debug.h>

#include <arpa/inet.h>

#include <nuttx/net/netconfig.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/netstats.h>
#include <nuttx/net/ip.h>
#include <nuttx/net/udp.h>

#include "devif/devif.h"
#include "utils/utils.h"
#include "udp/udp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IPv4BUF    ((struct ipv4_hdr_s *)&dev->d_buf[NET_LL_HDRLEN(dev)])
#define IPv6BUF    ((struct ipv6_hdr_s *)&dev->d_buf[NET_LL_HDRLEN(dev)])

#define UDPIPv4BUF ((struct udp_hdr_s *)&dev->d_buf[NET_LL_HDRLEN(dev) + IPv4_HDRLEN])
#define UDPIPv6BUF ((struct udp_hdr_s *)&dev->d_buf[NET_LL_HDRLEN(dev) + IPv6_HDRLEN])

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: udp_send
 *
 * Description:
 *   Set-up to send a UDP packet
 *
 * Parameters:
 *   dev  - The device driver structure to use in the send operation
 *   conn - The UDP "connection" structure holding port information
 *
 * Return:
 *   None
 *
 * Assumptions:
 *   Called from the interrupt level or with interrupts disabled.
 *
 ****************************************************************************/

void udp_send(FAR struct net_driver_s *dev, FAR struct udp_conn_s *conn)
{
  FAR struct udp_hdr_s *udp;

  nllvdbg("UDP payload: %d (%d) bytes\n", dev->d_sndlen, dev->d_len);

  if (dev->d_sndlen > 0)
    {
      /* Initialize the IP header. */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
      if (conn->domain == PF_INET)
#endif
        {
          /* Get pointers to the IPv4 header and the offset TCP header */

          FAR struct ipv4_hdr_s *ipv4 = IPv4BUF;

          DEBUGASSERT(IFF_IS_IPv4(dev->d_flags));
          udp = UDPIPv4BUF;

          /* Initialize the IPv4 header. */

          ipv4->vhl         = 0x45;
          ipv4->tos         = 0;
          ipv4->len[0]      = (dev->d_len >> 8);
          ipv4->len[1]      = (dev->d_len & 0xff);
          ++g_ipid;
          ipv4->ipid[0]     = g_ipid >> 8;
          ipv4->ipid[1]     = g_ipid & 0xff;
          ipv4->ipoffset[0] = 0;
          ipv4->ipoffset[1] = 0;
          ipv4->ttl         = conn->ttl;
          ipv4->proto       = IP_PROTO_UDP;

          net_ipv4addr_hdrcopy(ipv4->srcipaddr, &dev->d_ipaddr);
          net_ipv4addr_hdrcopy(ipv4->destipaddr, &conn->u.ipv4.raddr);

          /* Calculate IP checksum. */

          ipv4->ipchksum    = 0;
          ipv4->ipchksum    = ~ipv4_chksum(dev);

          /* The total length to send is the size of the application data
           * plus the IPv4 and UDP headers (and, eventually, the link layer
           * header)
           */

          dev->d_len = dev->d_sndlen + IPv4UDP_HDRLEN;
        }
#endif /* CONFIG_NET_IPv4 */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
      else
#endif
        {
          /* Get pointers to the IPv6 header and the offset TCP header */

          FAR struct ipv6_hdr_s *ipv6 = IPv6BUF;

          DEBUGASSERT(IFF_IS_IPv6(dev->d_flags));
          udp = UDPIPv6BUF;

          /* Initialize the IPv6 header.  Note that the IP length field
           * does not include the IPv6 IP header length.
           */

          ipv6->vtc         = 0x60;
          ipv6->tcf         = 0x00;
          ipv6->flow        = 0x00;
          ipv6->len[0]      = (dev->d_sndlen >> 8);
          ipv6->len[1]      = (dev->d_sndlen & 0xff);
          ipv6->proto       = IP_PROTO_UDP;
          ipv6->ttl         = conn->ttl;

          net_ipv6addr_copy(ipv6->srcipaddr, dev->d_ipv6addr);
          net_ipv6addr_copy(ipv6->destipaddr, conn->u.ipv6.raddr);

          /* The total length to send is the size of the application data
           * plus the IPv4 and UDP headers (and, eventually, the link layer
           * header)
           */

          dev->d_len = dev->d_sndlen + IPv6UDP_HDRLEN;
        }
#endif /* CONFIG_NET_IPv6 */

      /* Initialize the UDP header */

      udp->srcport     = conn->lport;
      udp->destport    = conn->rport;
      udp->udplen      = HTONS(dev->d_sndlen + UDP_HDRLEN);

#ifdef CONFIG_NET_UDP_CHECKSUMS
      /* Calculate UDP checksum. */

      udp->udpchksum   = 0;
      udp->udpchksum   = ~(udp_chksum(dev));
      if (udp->udpchksum == 0)
        {
          udp->udpchksum = 0xffff;
        }
#else
      udp->udpchksum   = 0;
#endif

      nllvdbg("Outgoing UDP packet length: %d\n", dev->d_len);

#ifdef CONFIG_NET_STATISTICS
      g_netstats.udp.sent++;
      g_netstats.ip.sent++;
#endif
    }
}
#endif /* CONFIG_NET && CONFIG_NET_UDP */
