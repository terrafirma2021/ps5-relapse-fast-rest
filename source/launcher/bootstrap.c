/* Copyright (C) 2024 John Törnblom

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <ps5/kernel.h>

#include "asset.h"
#include "elfldr.h"
#include "log.h"
// #include "notify.h"
#include "pt.h"

INCASSET(byepervisor_elf, "byepervisor_reborn_1360.elf");
INCASSET(socksrv_elf, "socksrv.elf");


/**
 *
 **/
static char* const byepervisor_argv[] = {"byepervisor_reborn_1360.elf", 0};
static char* const socksrv_argv[] = {"elfldr.elf", 0};

/**
 * We are running inside SceRedisServer. Spawn the pinned Byepervisor payload
 * directly in its own process so it remains alive over the S3 transition.
 **/
int
main() {
  pid_t mypid = getpid();
  uint8_t caps[16];
  uint64_t authid;
  intptr_t vnode;
  int ret;


  if(elfldr_sanity_check(byepervisor_elf, byepervisor_elf_size)) {
    LOG_PUTS("byepervisor ELF is corrupted");
    return -1;
  }
  if(elfldr_sanity_check(socksrv_elf, socksrv_elf_size)) {
    LOG_PUTS("headless ELF service is corrupted");
    return -1;
  }

  // backup my privileges
  if(!(vnode=kernel_get_proc_rootdir(mypid))) {
    LOG_PUTS("kernel_get_proc_rootdir failed");
    return -1;
  }
  if(kernel_get_ucred_caps(mypid, caps)) {
    LOG_PUTS("kernel_get_ucred_caps failed");
    return -1;
  }
  if(!(authid=kernel_get_ucred_authid(mypid))) {
    LOG_PUTS("kernel_get_ucred_authid failed");
    return -1;
  }

  // Launch Byepervisor in a new process. This lifetime boundary is required by
  // its post-resume polling loop and replaces the generic socket server.
  if(elfldr_raise_privileges(mypid)) {
    LOG_PUTS("Unable to raise privileges");
    ret = -1;
  } else {
    signal(SIGCHLD, SIG_IGN);
    // Start the UI-free loader before the autorest payload can suspend the
    // console. It is independent of Byepervisor and never opens Payload
    // Manager.
    ret = elfldr_spawn(-1, socksrv_argv, socksrv_elf, socksrv_elf_size);
    if (ret >= 0) {
      ret = elfldr_spawn(-1, byepervisor_argv, byepervisor_elf,
                         byepervisor_elf_size);
    }
  }

  // restore my privileges
  if(kernel_set_proc_jaildir(mypid, vnode)) {
    LOG_PUTS("kernel_set_proc_jaildir failed");
    ret = -1;
  }
  if(kernel_set_proc_rootdir(mypid, vnode)) {
    LOG_PUTS("kernel_set_proc_rootdir failed");
    ret = -1;
  }
  if(kernel_set_ucred_caps(mypid, caps)) {
    LOG_PUTS("kernel_set_ucred_caps failed");
    ret = -1;
  }
  if(kernel_set_ucred_authid(mypid, authid)) {
    LOG_PUTS("kernel_set_ucred_authid failed");
    ret = -1;
  }

  return ret;
}
