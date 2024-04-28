/****************************************************************************
 * arch/risc-v/src/sg2002/sg2002_irq.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include "riscv_internal.h"
#include "sg2002.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_irqinitialize
 ****************************************************************************/

void up_irqinitialize(void)
{
  sinfo("=============\n");
  /* Disable Machine interrupts */

  up_irq_save();

  /* Disable all global interrupts */

  putreg32(0x0, SG2002_PLIC_MIE0);
  putreg32(0x0, SG2002_PLIC_MIE1);

  /* Clear pendings in PLIC */

 uint32_t val = getreg32(SG2002_PLIC_MCLAIM);
  putreg32(val, SG2002_PLIC_MCLAIM);

  /* Colorize the interrupt stack for debug purposes */

#if defined(CONFIG_STACK_COLORATION) && CONFIG_ARCH_INTERRUPTSTACK > 15
  size_t intstack_size = (CONFIG_ARCH_INTERRUPTSTACK & ~15);
  riscv_stack_color(g_intstackalloc, intstack_size);
#endif

  /* Set priority for all global interrupts to 1 (lowest) */

  int id;

  for (id = (SG2002_IRQ_TEMPSENS - RISCV_IRQ_MEXT); id <= (SG2002_IRQ_MBOX - RISCV_IRQ_MEXT); id++)
    {
      putreg32(1, (uintptr_t)(SG2002_PLIC_PRIORITY + (4 * id)));
    }

  /* Set irq threshold to 0 (permits all global interrupts) */

  putreg32(0, SG2002_PLIC_MTHRESHOLD);

  /* Attach the common interrupt handler */

  riscv_exception_attach();

#ifndef CONFIG_SUPPRESS_INTERRUPTS

  /* And finally, enable interrupts */

  up_irq_enable();
#endif
}

/****************************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   Disable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_disable_irq(int irq)
{
  sinfo("~~~~~~~~~~+++===+++===irq: %d\n", irq);
  int extirq = 0;

  if (irq == RISCV_IRQ_MSOFT)
    {
      /* Read mstatus & clear machine software interrupt enable in mie */

      CLEAR_CSR(CSR_MIE, MIE_MSIE);
    }
  else if (irq == RISCV_IRQ_MTIMER)
    {
      /* Read mstatus & clear machine timer interrupt enable in mie */

      CLEAR_CSR(CSR_MIE, MIE_MTIE);
    }
  else if (irq > RISCV_IRQ_MEXT)
    {

      /* Clear enable bit for the irq */

      if (SG2002_IRQ_TEMPSENS <= extirq && extirq <= SG2002_IRQ_MBOX)
        {
          extirq = irq - RISCV_IRQ_MEXT;

          modifyreg32(SG2002_PLIC_MIE0 + (4 * (extirq / 32)),
                      1 << (extirq % 32), 0);
        }
      else
        {
          PANIC();
        }
    }
}

/****************************************************************************
 * Name: up_enable_irq
 *
 * Description:
 *   Enable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_enable_irq(int irq)
{
  sg2002_print_hex(irq);
  sinfo("irq: %d, in hex: 0x%08x\n", irq, irq);
  int extirq;

  if (irq == RISCV_IRQ_MSOFT)
    {
      /* Read mstatus & set machine software interrupt enable in mie */

      SET_CSR(CSR_MIE, MIE_MSIE);
    }
  else if (irq == RISCV_IRQ_MTIMER)
    {
      /* Read mstatus & set machine timer interrupt enable in mie */

      SET_CSR(CSR_MIE, MIE_MTIE);
    }
  else if (irq > RISCV_IRQ_MEXT)
    {
      /* Set enable bit for the irq */

      if (SG2002_IRQ_TEMPSENS <= irq && irq <= SG2002_IRQ_MBOX)
        {

          extirq = irq - RISCV_IRQ_MEXT;
          sinfo("extirq %d, in hex: 0x%08x\n", extirq, extirq);

          modifyreg32(SG2002_PLIC_MIE0 + (4 * (extirq / 32)),
                      0, 1 << (extirq % 32));
        }
      else
        {
          PANIC();
        }
    }
}

/****************************************************************************
 * Name: riscv_ack_irq
 *
 * Description:
 *   Acknowledge the IRQ
 *
 ****************************************************************************/

void riscv_ack_irq(int irq)
{
}

/****************************************************************************
 * Name: up_irq_enable
 *
 * Description:
 *   Return the current interrupt state and enable interrupts
 *
 ****************************************************************************/

irqstate_t up_irq_enable(void)
{
  sinfo("line: %d\n", __LINE__);
  irqstate_t oldstat;

  /* Enable MEIE (machine external interrupt enable) */

  /* TODO: should move to up_enable_irq() */

  SET_CSR(CSR_MIE, MIE_MEIE);

  /* Read mstatus & set machine interrupt enable (MIE) in mstatus */

  oldstat = READ_AND_SET_CSR(CSR_MSTATUS, MSTATUS_MIE);
  return oldstat;
}
