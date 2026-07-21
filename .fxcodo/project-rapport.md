[CRITICAL] ObjectsManager.h — destroy()
Problem: `std::lock_guard lk(_free)` is missing a semicolon before `_free.data().push_back(handle.id);`.
Why it matters: This is a syntax error. The whole translation unit fails to compile. Nothing runs.
Fix: Add `;` after `std::lock_guard lk(_free)`.

---

[CRITICAL] ObjectsManager.h — create() / destroy()
Problem: `_slots.size()`, `_slots.emplace_back()`, `_slots.at(id)` are called directly on `_slots`, which is a `th::Safe<std::vector<Slot<T>>>`. `Safe` only exposes `lock()`, `unlock()`, `try_lock()`, `data()`, `mtx()` — it has no `size()`, `emplace_back()`, or `at()`.
Why it matters: Compile error, "no member named ...". Code will not build at all.
Fix: Use `_slots.data().size()`, `_slots.data().emplace_back()`, `_slots.data().at(id)` everywhere.

---

[CRITICAL] ObjectsManager.h — get()
Problem: `get()` returns a raw pointer obtained under `_slots` lock, but the lock is released the instant the function returns. Nothing prevents another thread from calling `destroy()` on the same handle right after, which calls `slot.object.reset()`.
Why it matters: Caller ends up dereferencing a freed object — classic use-after-free, will segfault or corrupt memory under any real concurrency.
Fix: Either return a locked accessor/shared_ptr with proper lifetime guarantee, or make callers hold the manager lock for the duration of use (defeats purpose), or use reference counting per-slot.

---

[CRITICAL] ObjectsManager.h — Slot<T>::object / destroy()
Problem: `Slot<T>` stores `std::unique_ptr<T> object`, but objects are created as derived type `S` via `std::make_unique<S>(...)` and stored/destroyed through the base `T*`. If `T` has no virtual destructor, `slot.object.reset()` invokes `~T()` instead of `~S()`.
Why it matters: Undefined behavior — derived destructor and members never run, heap corruption possible, crash likely eventually (double free / leaked resources / corrupted allocator metadata).
Fix: Require/enforce `T` to have a virtual destructor (static_assert `std::has_virtual_destructor_v<T>`).

---

[HIGH] ObjectsManager.h — create()
Problem: `std::make_unique<S>(std::forward<Args>(args)...)` is called while holding the `_slots` lock (non-recursive mutex). If `S`'s constructor calls back into the same `ObjectsManager` (create/get/destroy) on the same thread, the thread deadlocks on its own mutex.
Why it matters: Non-recursive `std::mutex` relocking from the same thread is undefined behavior (typically hangs forever, not a clean crash — worse to debug).
Fix: Don't hold the lock during arbitrary user-code execution; reserve the slot under lock, release, construct object, then commit under lock again — or document/forbid reentrancy.

---

[HIGH] ObjectsManager.h — create()
Problem: If `std::make_unique<S>(...)` throws, the slot id was already consumed (either a brand-new emplaced slot or popped from `_free`), but `slot.object` stays null and the id is never returned to `_free` nor rolled back.
Why it matters: Permanent slot leak on every failed construction; over time exhausts the vector/generation space uselessly.
Fix: Wrap construction in try/catch, and on exception push the id back to `_free` (or roll back the emplace) before rethrowing.

---

[MEDIUM] ObjectsManager.h — create()
Problem: `unsigned int id = _slots.data().size();` truncates a `size_t` into `unsigned int`.
Why it matters: On platforms/uses where slot count exceeds 4294967295, id wraps silently, corrupting handle-to-slot mapping (writes to wrong slot). Unlikely in practice, but a real latent bug.
Fix: Use a wide enough type or assert size doesn't exceed `UINT_MAX`.

---

[LOW] ObjectsManager.h — Handle<T>::valid()
Problem: `generation != 0` is used as "not invalid", but a default-constructed `Handle` has `generation = 0` while a freshly created slot starts at `generation = 1`. Fine today, but nothing stops external code from constructing a `Handle{someId, 0}` that looks invalid while `id` isn't the sentinel.
Why it matters: Fragile invariant relies on two unrelated fields both being "correct" for validity; easy to break with future edits (e.g., changing default generation).
Fix: Make Handle constructible only via the manager, or add a single explicit "valid" flag instead of dual sentinel logic.