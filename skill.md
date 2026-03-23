# SKILL: dist-td (TDX Runtime Model)

이 문서는 TDX 환경에서 MN이 어떻게 동작해야 하는지 정의한다.

---

## 1. Memory Architecture

MN 내부 메모리는 다음과 같이 구성된다:

- shared region
  - RDMA exposed
  - PRIME / CACHE / BACKUP slot 포함

- private region
  - metadata, lock, eviction state

---

## 2. RDMA Path

RDMA는 shared region만 접근한다:

- READ → shared slot
- WRITE → shared slot

MN CPU는 관여하지 않는다 (one-sided)

---

## 3. TDCALL Usage Rules

### 반드시 필요한 경우

- memory conversion
  - TDG.MEM.PAGE.ACCEPT
  - TDG.MEM.PAGE.RELEASE

- #VE handling
- IO / MMIO emulation

---

### 절대 사용하지 않는 경우

- RDMA data path
- slot read/write

---

## 4. TDCALL Wrapper Pattern

모든 tdcall은 retry loop로 감싸야 한다:

```c
while (1) {
    ret = tdcall(...);

    if (ret == SUCCESS)
        break;

    if (ret == BUSY || ret == INTERRUPTED)
        continue;

    panic();
}