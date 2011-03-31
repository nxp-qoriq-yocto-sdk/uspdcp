
#ifndef SEC_MEM_MANAGEMENT_H
#define SEC_MEM_MANAGEMENT_H

#ifdef __cplusplus
/* *INDENT-OFF* */

extern "C"{
/* *INDENT-ON* */
#endif

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/
// For dma_mem library, Freescale's custom memory management mechanism
// TODO: replace with ALU's memory management mechanism !!!!!
#include "compat.h"

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
/** @brief      Translates the virtual address to physical address.
 *
 * @param [in] address  The virtual address which has to be mapped.
 *
 * @retval      Physical address which the passed virtual address maps to
 *              A value of -1 is returned if the passed virtual address
 *              doesn't map to any physical address.
 */
// TODO: replace with ALU's memory management mechanism !!!!!
#define sec_vtop(virt_address) dma_mem_vtop(virt_address)


/** @brief      Translates the physical address to virtual address.
 *
 * @param [in]  address The physical address which has to be mapped.
 *
 * @retval      Virtual address to which the passed physical address maps onto.
 *              NULL is returned if the passed physical address doesn't map to any virtual address.
 */
// TODO: replace with ALU's memory management mechanism !!!!!
#define sec_ptov(phys_address) dma_mem_ptov(phys_address)

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

/*================================================================================================*/

/*================================================================================================*/

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //SEC_MEM_MANAGEMENT_H
