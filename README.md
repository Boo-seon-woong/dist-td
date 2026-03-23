# dist-td (TDX-enabled MN)

이 버전의 dist-td는 MN을 Intel TDX Trust Domain 내부에서 실행하도록 확장된 구조다.

기존과 달리 MN은 shared/private memory를 구분하며, RDMA는 shared memory를 통해서만 접근된다.

---

## 핵심 변경점

### 1. Memory Model

MN 내부 메모리는 두 가지로 나뉜다:

- Private Memory
  - TD 내부에서만 접근 가능
  - metadata, control state, eviction state

- Shared Memory
  - RDMA 및 VMM 접근 가능
  - PRIME / CACHE / BACKUP slot 모두 여기에 존재

---

### 2. RDMA 접근 방식

- RDMA는 **shared memory region만 접근 가능**
- slot body read/write는 기존과 동일하게 one-sided RDMA 사용
- MN CPU는 여전히 control path(CAS 등)만 처리

---

### 3. TDCALL의 역할

TDCALL은 다음 경우에만 사용된다:

- shared ↔ private memory conversion
- TD-VMM communication (#VE handling 등)
- MMIO/IO emulation

RDMA 데이터 path에는 사용되지 않는다.

---

### 4. MN 역할 재정의

기존:
> MN = passive memory appliance

변경:
> MN = TDX-isolated memory appliance with shared memory exposure

---

## 아키텍처

CN (untrusted)
  ↓ RDMA
Shared Memory (MN inside TD)
  ↓
MN Private Logic (TD 내부)

---

## Slot Layout

모든 slot은 shared memory에 존재한다:

PRIME / CACHE / BACKUP

slot format은 기존과 동일하다.

---

## Security Model

- shared memory는 untrusted
- CN은 여전히 암호화/MAC 책임 유지
- MN은 plaintext 해석하지 않음

---

## 중요한 제약

- shared memory 접근 시 secure computation 금지
- #VE handler 내부에서 secret 사용 금지
- TDCALL은 retry loop 기반으로 호출해야 함