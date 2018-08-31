/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Handle_h
#define Handle_h

#include "platform/heap/Heap.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/heap/InlinedGlobalMarkingVisitor.h"
#include "platform/heap/Member.h"
#include "platform/heap/Persistent.h"
#include "platform/heap/ThreadState.h"
#include "platform/heap/TraceTraits.h"
#include "platform/heap/Visitor.h"
#include "wtf/Allocator.h"
 #include "wtf/RefCounted.h"

#if defined(LEAK_SANITIZER)
#include "wtf/LeakAnnotations.h"
#endif

namespace blink {

// LEAK_SANITIZER_DISABLED_SCOPE: all allocations made in the current scope
// will be exempted from LSan consideration.
//
// TODO(sof): move this to wtf/LeakAnnotations.h (LeakSanitizer.h?) once
// wtf/ can freely call upon Oilpan functionality.
#if defined(LEAK_SANITIZER)
class LeakSanitizerDisableScope {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(LeakSanitizerDisableScope);

 public:
  LeakSanitizerDisableScope() {
    __lsan_disable();
    if (ThreadState::current())
      ThreadState::current()->enterStaticReferenceRegistrationDisabledScope();
  }

  ~LeakSanitizerDisableScope() {
    __lsan_enable();
    if (ThreadState::current())
      ThreadState::current()->leaveStaticReferenceRegistrationDisabledScope();
  }
};
#define LEAK_SANITIZER_DISABLED_SCOPE \
  LeakSanitizerDisableScope lsanDisabledScope
#else
#define LEAK_SANITIZER_DISABLED_SCOPE
#endif

//zhangyq
#if ENABLE(OILPAN)
#define PassRefPtrWillBeRawPtr WTF::RawPtr
#define RefCountedWillBeGarbageCollected blink::GarbageCollected
#define RefCountedWillBeGarbageCollectedFinalized blink::GarbageCollectedFinalized
#define RefCountedWillBeRefCountedGarbageCollected blink::RefCountedGarbageCollected
#define RefCountedGarbageCollectedWillBeGarbageCollectedFinalized blink::GarbageCollectedFinalized
#define RefCountedWillBeNoBase blink::DummyBase
#define RefCountedGarbageCollectedWillBeNoBase blink::DummyBase
#define ThreadSafeRefCountedWillBeGarbageCollected blink::GarbageCollected
#define ThreadSafeRefCountedWillBeGarbageCollectedFinalized blink::GarbageCollectedFinalized
#define PersistentWillBeMember blink::Member
#define CrossThreadPersistentWillBeMember blink::Member
#define RefPtrWillBePersistent blink::Persistent
#define RefPtrWillBeRawPtr WTF::RawPtr
#define RefPtrWillBeMember blink::Member
#define RefPtrWillBeWeakMember blink::WeakMember
#define RefPtrWillBeWeakPersistent blink::WeakPersistent
#define RefPtrWillBeCrossThreadPersistent blink::CrossThreadPersistent
#define RawPtrWillBeMember blink::Member
#define RawPtrWillBePersistent blink::Persistent
#define RawPtrWillBeWeakMember blink::WeakMember
#define RawPtrWillBeWeakPersistent blink::WeakPersistent
#define RawPtrWillBeUntracedMember blink::UntracedMember
#define OwnPtrWillBeCrossThreadPersistent blink::CrossThreadPersistent
#define OwnPtrWillBeMember blink::Member
#define OwnPtrWillBePersistent blink::Persistent
#define OwnPtrWillBeRawPtr WTF::RawPtr
#define PassOwnPtrWillBeRawPtr WTF::RawPtr
#define WeakPtrWillBeCrossThreadWeakPersistent blink::CrossThreadWeakPersistent
#define WeakPtrWillBeMember blink::Member
#define WeakPtrWillBeRawPtr WTF::RawPtr
#define WeakPtrWillBeWeakMember blink::WeakMember
#define WeakPtrWillBeWeakPersistent blink::WeakPersistent
#define NoBaseWillBeGarbageCollected blink::GarbageCollected
#define NoBaseWillBeGarbageCollectedFinalized blink::GarbageCollectedFinalized
#define NoBaseWillBeRefCountedGarbageCollected blink::RefCountedGarbageCollected
#define WillBeHeapHashMap blink::HeapHashMap
#define WillBePersistentHeapHashMap blink::PersistentHeapHashMap
#define WillBeHeapHashSet blink::HeapHashSet
#define WillBePersistentHeapHashSet blink::PersistentHeapHashSet
#define WillBeHeapLinkedHashSet blink::HeapLinkedHashSet
#define WillBePersistentHeapLinkedHashSet blink::PersistentHeapLinkedHashSet
#define WillBeHeapListHashSet blink::HeapListHashSet
#define WillBePersistentHeapListHashSet blink::PersistentHeapListHashSet
#define WillBeHeapVector blink::HeapVector
#define WillBePersistentHeapVector blink::PersistentHeapVector
#define WillBeHeapDeque blink::HeapDeque
#define WillBePersistentHeapDeque blink::PersistentHeapDeque
#define WillBeHeapHashCountedSet blink::HeapHashCountedSet
#define WillBePersistentHeapHashCountedSet blink::PersistentHeapHashCountedSet
#define WillBeGarbageCollectedMixin blink::GarbageCollectedMixin
#define WillBeHeapSupplement blink::HeapSupplement
#define WillBeHeapSupplementable blink::HeapSupplementable
#define WillBeHeapTerminatedArray blink::HeapTerminatedArray
#define WillBeHeapTerminatedArrayBuilder blink::HeapTerminatedArrayBuilder
#define WillBeHeapLinkedStack blink::HeapLinkedStack
#define PersistentHeapHashMapWillBeHeapHashMap blink::HeapHashMap
#define PersistentHeapHashSetWillBeHeapHashSet blink::HeapHashSet
#define PersistentHeapDequeWillBeHeapDeque blink::HeapDeque
#define PersistentHeapVectorWillBeHeapVector blink::HeapVector

template<typename T> T* adoptRefWillBeNoop(T* ptr)
{
    static const bool isGarbageCollected = IsGarbageCollectedType<T>::value;
    static const bool isRefCounted = WTF::IsSubclassOfTemplate<typename std::remove_const<T>::type, RefCounted>::value;
    static_assert(isGarbageCollected && !isRefCounted, "T needs to be a non-refcounted, garbage collected type.");
    return ptr;
}

template<typename T> T* adoptPtrWillBeNoop(T* ptr)
{
    static const bool isGarbageCollected = IsGarbageCollectedType<T>::value;
    static_assert(isGarbageCollected, "T needs to be a garbage collected type.");
    return ptr;
}

#define USING_FAST_MALLOC_WILL_BE_REMOVED(type) // do nothing when oilpan is enabled.
#define USING_FAST_MALLOC_WITH_TYPE_NAME_WILL_BE_REMOVED(type)
#define DECLARE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(type) // do nothing
#define DECLARE_EMPTY_VIRTUAL_DESTRUCTOR_WILL_BE_REMOVED(type) // do nothing
#define DEFINE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(type) // do nothing

#define DEFINE_STATIC_REF_WILL_BE_PERSISTENT(type, name, arguments) \
    static type* name = (new Persistent<type>(arguments))->get();

#else // !ENABLE(OILPAN)

#define PassRefPtrWillBeRawPtr WTF::PassRefPtr
#define RefCountedWillBeGarbageCollected WTF::RefCounted
#define RefCountedWillBeGarbageCollectedFinalized WTF::RefCounted
#define RefCountedWillBeRefCountedGarbageCollected WTF::RefCounted
#define RefCountedGarbageCollectedWillBeGarbageCollectedFinalized blink::RefCountedGarbageCollected
#define RefCountedWillBeNoBase WTF::RefCounted
#define RefCountedGarbageCollectedWillBeNoBase blink::RefCountedGarbageCollected
#define ThreadSafeRefCountedWillBeGarbageCollected WTF::ThreadSafeRefCounted
#define ThreadSafeRefCountedWillBeGarbageCollectedFinalized WTF::ThreadSafeRefCounted
#define PersistentWillBeMember blink::Persistent
#define CrossThreadPersistentWillBeMember blink::CrossThreadPersistent
#define RefPtrWillBePersistent WTF::RefPtr
#define RefPtrWillBeRawPtr WTF::RefPtr
#define RefPtrWillBeMember WTF::RefPtr
#define RefPtrWillBeWeakMember WTF::RefPtr
#define RefPtrWillBeWeakPersistent WTF::RefPtr
#define RefPtrWillBeCrossThreadPersistent WTF::RefPtr
#define RawPtrWillBeMember WTF::RawPtr
#define RawPtrWillBePersistent WTF::RawPtr
#define RawPtrWillBeWeakMember WTF::RawPtr
#define RawPtrWillBeWeakPersistent WTF::RawPtr
#define RawPtrWillBeUntracedMember WTF::RawPtr
#define OwnPtrWillBeCrossThreadPersistent WTF::OwnPtr
#define OwnPtrWillBeMember WTF::OwnPtr
#define OwnPtrWillBePersistent WTF::OwnPtr
#define OwnPtrWillBeRawPtr WTF::OwnPtr
#define PassOwnPtrWillBeRawPtr WTF::PassOwnPtr
#define WeakPtrWillBeCrossThreadWeakPersistent WTF::WeakPtr
#define WeakPtrWillBeMember WTF::WeakPtr
#define WeakPtrWillBeRawPtr WTF::WeakPtr
#define WeakPtrWillBeWeakMember WTF::WeakPtr
#define WeakPtrWillBeWeakPersistent WTF::WeakPtr
#define NoBaseWillBeGarbageCollected blink::DummyBase
#define NoBaseWillBeGarbageCollectedFinalized blink::DummyBase
#define NoBaseWillBeRefCountedGarbageCollected blink::DummyBase
#define WillBeHeapHashMap WTF::HashMap
#define WillBePersistentHeapHashMap WTF::HashMap
#define WillBeHeapHashSet WTF::HashSet
#define WillBePersistentHeapHashSet WTF::HashSet
#define WillBeHeapLinkedHashSet WTF::LinkedHashSet
#define WillBePersistentLinkedHeapHashSet WTF::LinkedHashSet
#define WillBeHeapListHashSet WTF::ListHashSet
#define WillBePersistentListHeapHashSet WTF::ListHashSet
#define WillBeHeapVector WTF::Vector
#define WillBePersistentHeapVector WTF::Vector
#define WillBeHeapDeque WTF::Deque
#define WillBePersistentHeapDeque WTF::Deque
#define WillBeHeapHashCountedSet WTF::HashCountedSet
#define WillBePersistentHeapHashCountedSet WTF::HashCountedSet
#define WillBeGarbageCollectedMixin blink::DummyBase<void>
#define WillBeHeapSupplement blink::Supplement
#define WillBeHeapSupplementable blink::Supplementable
#define WillBeHeapTerminatedArray WTF::TerminatedArray
#define WillBeHeapTerminatedArrayBuilder WTF::TerminatedArrayBuilder
#define WillBeHeapLinkedStack WTF::LinkedStack
#define PersistentHeapHashMapWillBeHeapHashMap blink::PersistentHeapHashMap
#define PersistentHeapHashSetWillBeHeapHashSet blink::PersistentHeapHashSet
#define PersistentHeapDequeWillBeHeapDeque blink::PersistentHeapDeque
#define PersistentHeapVectorWillBeHeapVector blink::PersistentHeapVector

template<typename T> PassRefPtrWillBeRawPtr<T> adoptRefWillBeNoop(T* ptr) { return adoptRef(ptr); }
template<typename T> PassOwnPtrWillBeRawPtr<T> adoptPtrWillBeNoop(T* ptr) { return adoptPtr(ptr); }

#define USING_FAST_MALLOC_WILL_BE_REMOVED(type) USING_FAST_MALLOC(type)
#define USING_FAST_MALLOC_WITH_TYPE_NAME_WILL_BE_REMOVED(type) USING_FAST_MALLOC_WITH_TYPE_NAME(type)
#define DECLARE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(type) \
    public:                                            \
        ~type();                                       \
    private:
#define DECLARE_EMPTY_VIRTUAL_DESTRUCTOR_WILL_BE_REMOVED(type) \
    public:                                                    \
        virtual ~type();                                       \
    private:

#define DEFINE_EMPTY_DESTRUCTOR_WILL_BE_REMOVED(type) \
    type::~type() { }

#define DEFINE_STATIC_REF_WILL_BE_PERSISTENT(type, name, arguments) \
    DEFINE_STATIC_REF(type, name, arguments)

#endif // ENABLE(OILPAN)
//zhangyq add end

}  // namespace blink

#endif
