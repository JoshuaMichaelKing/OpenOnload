/*
** Copyright 2005-2017  Solarflare Communications Inc.
**                      7505 Irvine Center Drive, Irvine, CA 92618, USA
** Copyright 2002-2005  Level 5 Networks Inc.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of version 2.1 of the GNU Lesser General Public
** License as published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
*/

/****************************************************************************
 * Copyright 2002-2005: Level 5 Networks Inc.
 * Copyright 2005-2008: Solarflare Communications Inc,
 *                      9501 Jeronimo Road, Suite 250,
 *                      Irvine, CA 92618, USA
 *
 * Maintained by Solarflare Communications
 *  <linux-xen-drivers@solarflare.com>
 *  <onload-dev@solarflare.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, incorporated herein by reference.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ****************************************************************************
 */

/*
 * \author  djr
 *  \brief  Really-and-truely-honestly internal stuff for libef.
 *   \date  2004/06/13
 */

/*! \cidoxg_include_ci_ul */
#ifndef __CI_EF_VI_INTERNAL_H__
#define __CI_EF_VI_INTERNAL_H__


/**********************************************************************
 * Headers
 */

#include <etherfabric/ef_vi.h>
#include <etherfabric/internal/internal.h>
#include <etherfabric/pd.h>
#include "sysdep.h"
#include "ef_vi_falcon.h"
#include "ef_vi_ef10.h"


/**********************************************************************
 * Initialisation state.
 */

#define EF_VI_INITED_NIC		0x1
#define EF_VI_INITED_IO			0x2
#define EF_VI_INITED_RXQ		0x4
#define EF_VI_INITED_TXQ		0x8
#define EF_VI_INITED_EVQ		0x10
#define EF_VI_INITED_TIMER		0x20
#define EF_VI_INITED_RX_TIMESTAMPING	0x40
#define EF_VI_INITED_TX_TIMESTAMPING	0x80
#define EF_VI_INITED_OUT_FLAGS		0x100


/**********************************************************************
 * Debugging.
 */

#define __EF_VI_BUILD_ASSERT_NAME(_x) __EF_VI_BUILD_ASSERT_ILOATHECPP(_x)
#define __EF_VI_BUILD_ASSERT_ILOATHECPP(_x)  __EF_VI_BUILD_ASSERT__##_x
#define EF_VI_BUILD_ASSERT(e)                                           \
  { typedef char __EF_VI_BUILD_ASSERT_NAME(__LINE__)[(e) ? 1 : -1]; }


#ifdef NDEBUG
# define EF_VI_ASSERT(x)   do{}while(0)
# ifdef __KERNEL__
#  define EF_VI_BUG_ON(x)  WARN_ON(x)
# else
#  define EF_VI_BUG_ON(x)  do{}while(0)
# endif
#else
# define EF_VI_ASSERT(x)  BUG_ON(!(x))
# define EF_VI_BUG_ON(x)  BUG_ON(x)
#endif


/* *********************************************************************
 * Miscellaneous goodies
 */

#ifdef NDEBUG
# define EF_VI_DEBUG(x)
#else
# define EF_VI_DEBUG(x)            x
#endif

#define EF_VI_ROUND_UP(i, align)   (((i)+(align)-1u) & ~((align)-1u))
#define EF_VI_ALIGN_FWD(p, align)  (((p)+(align)-1u) & ~((align)-1u))
#define EF_VI_ALIGN_BACK(p, align) ((p) & ~((align)-1u))
#define EF_VI_PTR_ALIGN_BACK(p, align)					\
  ((char*)EF_VI_ALIGN_BACK(((intptr_t)(p)), ((intptr_t)(align))))
#define EF_VI_IS_POW2(x)           ((x) && ! ((x) & ((x) - 1)))


/* This macro must be defined to the same value as EFHW_NIC_PAGE_SIZE
 * in ci/efhw/common.h. Only defined numerically so that there be no
 * dependency on that header here
 */
#define EF_VI_PAGE_SIZE   4096
#define EF_VI_PAGE_SHIFT  12

#define EF_VI_TX_TIMESTAMP_TS_NSEC_INVALID (1u<<30)

/* required for CI_PAGE_SIZE and related things */
#include "ci/compat.h"

/*----------------------------------------------------------------------------
 *
 * Helpers to turn bit shifts into dword shifts and check that the bit fields 
 * haven't overflown the dword etc. Aim is to preserve consistency with the 
 * autogenerated headers - once stable we could hard code.
 *
 *---------------------------------------------------------------------------*/

/* mask constructors */
#define __EFVI_MASK(WIDTH,T)  ((((T)1) << (WIDTH)) - 1)
#define __EFVI_MASK32(WIDTH)  __EFVI_MASK((WIDTH),uint32_t)
#define __EFVI_MASK64(WIDTH)  __EFVI_MASK((WIDTH),uint64_t)

#define __EFVI_MASKFIELD32(LBN, WIDTH)   ((uint32_t)			\
					  (__EFVI_MASK32(WIDTH) << (LBN)))

/* constructors for fields which span the first and second dwords */
#define __LW(LBN) (32 - LBN)
#define LOW(v, LBN, WIDTH)   ((uint32_t)                                \
                              (((v) & __EFVI_MASK64(__LW((LBN)))) << (LBN)))
#define HIGH(v, LBN, WIDTH)  ((uint32_t)(((v) >> __LW((LBN))) &         \
                                         __EFVI_MASK64((WIDTH - __LW((LBN))))))
/* constructors for fields within the second dword */
#define __DW2(LBN) 	  ((LBN) - 32)

/* constructors for fields which span the second and third dwords */
#define __LW2(LBN) (64 - LBN)
#define LOW2(v, LBN, WIDTH) ((uint32_t)                                 \
                             (((v) & __EFVI_MASK64(__LW2((LBN)))) << ((LBN) - 32)))
#define HIGH2(v, LBN, WIDTH)  ((uint32_t)                               \
                               (((v) >> __LW2((LBN))) & __EFVI_MASK64((WIDTH - __LW2((LBN))))))

/* constructors for fields within the third dword */
#define __DW3(LBN) 	  ((LBN) - 64)

				
/* constructors for fields which span the third and fourth dwords */
#define __LW3(LBN) (96 - LBN)
#define LOW3(v, LBN, WIDTH)   ((uint32_t)                               \
                               (((v) & __EFVI_MASK64(__LW3((LBN)))) << ((LBN) - 64)))
#define HIGH3(v, LBN, WIDTH)  ((unit32_t)                               \
                               (((v) >> __LW3((LBN))) & __EFVI_MASK64((WIDTH - __LW3((LBN))))))

/* constructors for fields within the fourth dword */
#define __DW4(LBN) 	  ((LBN) - 96)

/* checks that the autogenerated headers our consistent with our model */
#define WIDTHCHCK(a, b)  EF_VI_BUG_ON((a) != (b))
#define RANGECHCK(v, WIDTH)                                             \
  EF_VI_BUG_ON(((uint64_t)(v) & ~(__EFVI_MASK64((WIDTH)))) != 0)

/* fields within the first dword */
#define DWCHCK(LBN, WIDTH) EF_VI_BUG_ON(((LBN) < 0) || (((LBN)+(WIDTH)) > 32))

/* fields which span the first and second dwords */
#define LWCHK(LBN, WIDTH)  EF_VI_BUG_ON(WIDTH < __LW(LBN))


/**********************************************************************
 * Extracting bit fields.
 */

#define QWORD_GET_U(field, val)                 \
  ((unsigned) CI_QWORD_FIELD(val, field))

#define QWORD_TEST_BIT(field, val)              \
  ( !!CI_QWORD_FIELD(val, field) )


/**********************************************************************
 * Packed stream mode parameters.
 */

/* The gap left after each packet in a packed stream buffer. */
#define EF_VI_PS_PACKET_GAP              64

/* Firmware aligns DMAs onto this boundary. */
#define EF_VI_PS_ALIGNMENT               64

/* The negative offset from the start of a packet's DMA to the start of the
 * metadata.
 *
 * This is set larger than the size of the metadata to give space for an empty
 * option record. While there is space available between the DMA start location
 * and the packet contents, we avoid using that since that would mean modifying
 * two cache lines rather than one.
 */
#define EF_VI_PS_METADATA_OFFSET                \
  (sizeof(ef_packed_stream_packet) + sizeof(uint32_t))

/* The amount of space we leave at the start of each buffer before the
 * first DMA.  Needs to be enough space for ef_packed_stream_packet, plus
 * we leave some more space for application metadata.  (NB. We could make
 * this a runtime runable if needed).
 *
 * Firmware requires this be a multiple of EF_VI_PS_ALIGNMENT, and also
 * important for it to be a multiple of EF_VI_DMA_ALIGN.
 */
#define EF_VI_PS_DMA_START_OFFSET        256

/* Doxbox SF-112241-TC: One credit is consumed on crossing a 64KB boundary
 * in buffer space.
 */
#define EF_VI_PS_SPACE_PER_CREDIT        0x10000


/* ******************************************************************** 
 */

extern void falcon_vi_init(ef_vi*) EF_VI_HF;

extern void falcon_ef_eventq_prime(ef_vi*);
extern int falcon_ef_eventq_poll(ef_vi*, ef_event*, int evs_len);

extern void falcon_ef_eventq_timer_prime(ef_vi*, unsigned v);
extern void falcon_ef_eventq_timer_run(ef_vi*, unsigned v);
extern void falcon_ef_eventq_timer_clear(ef_vi*);
extern void falcon_ef_eventq_timer_zero(ef_vi*);

extern void ef10_vi_init(ef_vi*) EF_VI_HF;

extern void ef10_ef_eventq_prime(ef_vi*);
extern int ef10_ef_eventq_poll(ef_vi*, ef_event*, int evs_len);

extern void ef10_ef_eventq_timer_prime(ef_vi*, unsigned v);
extern void ef10_ef_eventq_timer_run(ef_vi*, unsigned v);
extern void ef10_ef_eventq_timer_clear(ef_vi*);
extern void ef10_ef_eventq_timer_zero(ef_vi*);

extern int ef_pd_cluster_free(ef_pd*, ef_driver_handle);

extern void ef_vi_packed_stream_update_credit(ef_vi* vi);

extern void ef_vi_set_intf_ver(char* intf_ver, size_t len);

enum ef_vi_capability;
extern int
ef_pd_capabilities_get(ef_driver_handle handle, ef_pd* pd,
                       ef_driver_handle pd_dh, enum ef_vi_capability cap,
                       unsigned long* value);


#endif  /* __CI_EF_VI_INTERNAL_H__ */
