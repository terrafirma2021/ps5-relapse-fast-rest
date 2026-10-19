# PS5 ReLapse Fast Rest

One-action PS5 firmware 13.60 chain combining ReLapse, a persistent headless ELF
loader, and an optimized Byepervisor Reborn handoff into automatic Rest Mode.

This project is an integration and optimization of the credited upstream work;
it is not a claim of authorship over ReLapse or Byepervisor Reborn.

## What it does

```text
Settings → User's Guide
  → WebKit userland exploit
  → ReLapse kernel R/W
  → persistent headless ELF service on TCP 9021
  → prepare and arm the optimized Byepervisor resume hook
  → automatic Rest Mode
  → Byepervisor's hypervisor payload executes during S3 wake
  → return with the hypervisor defeated and port 9021 available
```

There is no payload menu, Payload Manager launch, browser interaction, or
standalone kstuff in this release.

## Requirements

- PS5 system software **13.60**
- A PC and PS5 on the same local network
- Python 3.10 or newer
- The PS5 must be configured to reach the PC-hosted User's Guide

## Run

Download `ps5-relapse-fast-rest-v1.0.0.py` from the release and run:

```powershell
python ps5-relapse-fast-rest-v1.0.0.py --no-dns --ip YOUR_PC_IP
```

Then open **Settings → User's Guide** on the PS5. The console enters Rest Mode
automatically after the Byepervisor resume hook is armed. The actual hypervisor
breakout runs in the resume path when the PS5 is woken; it is deliberately not
performed before sleep.

> Never retry ReLapse after a kernel attempt in the same boot. Fully reboot the
> PS5 before trying again.

## Measured timing

Measured on one retail PS5 running 13.60. Results vary with WebKit heap layout,
networking, and whether the first WebKit attempt succeeds.

### Fastest measured run

| Stage | Fast Rest v1.0.0 |
|---|---:|
| Page open → WebKit ARW | 0.55 s |
| Page open → ReLapse kernel R/W established | 1.54 s |
| Persistent loader spawn | 19 ms |
| Byepervisor process spawn | 16 ms |
| Byepervisor process activation | 223 ms |
| Pre-sleep hook setup | 10 ms |
| Standby request | 3 ms |
| Native post-ReLapse path subtotal | 272 ms |
| **Page open → automatic Rest Mode** | **~1.8 s** |

The timing is a best observed first-attempt result, not a guarantee. It measures
time **to Rest Mode**, not time to complete the hypervisor exploit. Byepervisor
does its decisive work during wake, which is a major reason the pre-sleep path
is so short.

### Stock/reference measurements on the same console

The reference ReLapse path reached `Kernel: payloads loaded` at **8.60 s** and
finished descriptor cleanup at **8.83 s**. It then still required a separate
Byepervisor delivery/launch and a manual Rest Mode transition.

The old Byepervisor kernel-text permission pass took approximately **444 ms** by
walking 4 KiB pages repeatedly. The page-size-aware walker used here takes
approximately **10 ms**. In the integrated build, all measured pre-sleep native
overhead after ReLapse is about **272 ms**; 223 ms of that is process activation,
while actual hook setup is only 10 ms.

| Same-console comparison | Stock/separate path | Fast Rest v1.0.0 |
|---|---:|---:|
| ReLapse through payload load | 8.60 s | 1.54 s to kernel R/W in the fastest run |
| ReLapse through final cleanup | 8.83 s | Integrated into the short handoff |
| Byepervisor permission walk | ~444 ms | ~10 ms |
| Native loader + Byepervisor + standby | Separate operator-controlled steps | ~272 ms |
| Hypervisor breakout | On S3 resume | On S3 resume |

## Compared with stock ReLapse + Byepervisor Reborn

| Behavior | Stock projects used separately | Fast Rest v1.0.0 |
|---|---|---|
| Operator flow | Run ReLapse, transfer/start loader and Byepervisor, enter Rest Mode manually | Open User's Guide once |
| ELF delivery | Separate transfer/launcher step | Embedded and started directly from memory |
| Post-launch loader | Depends on the selected external loader | Headless service starts automatically and survives Rest Mode |
| Byepervisor setup | Original page-by-page kernel-text permission walk | Page-size-aware PDE/PTE walk |
| Measured permission walk | ~444 ms | ~10 ms |
| Rest Mode | Manual after hook setup | Requested only after the resume hook is armed |
| UI | Depends on host/loader choices | No menu or interactive payload selection |
| Work done before sleep | Prepare hook, then wait for manual sleep | Prepare hook and request standby immediately |
| Work done during wake | Hypervisor payload executes in `startup_mp` resume path | Same exploit point; integrated automatically |
| Wake result | Hypervisor defeat; loader availability depends on setup | Hypervisor defeated with TCP 9021 restored |

The speedup comes primarily from direct in-memory spawning, eliminating manual
handoffs, leaving the actual Byepervisor breakout at its intended S3 resume
point, and replacing repeated page-table walks with a page-size-aware walker. The
ReLapse race strategy and firmware offsets are not made more aggressive.

## Integrity

Published SHA-256 values are in [`checksums.txt`](checksums.txt).

## Source layout

- `source/launcher/` — combined headless loader/bootstrap source
- `source/byepervisor/` — Byepervisor integration and optimized walker source
- `dist/` — ready-to-run host and pinned Byepervisor ELF

Builds require the PS5 Payload SDK. See the source Makefiles for the expected
`PS5_PAYLOAD_SDK` environment variable.

## Credits

- **ReLapse / ps5-kexp / WebKit remote loader:** itsPLK and contributors
- **Byepervisor Reborn:** PS5Dev and contributors
- **PS5 ELF loader:** John Törnblom and contributors
- **PS5 Payload SDK:** ps5-payload-dev contributors

See [`THIRD_PARTY.md`](THIRD_PARTY.md) for upstream links and licensing notes.

## License

GPL-3.0. Third-party components remain under their respective licenses.
