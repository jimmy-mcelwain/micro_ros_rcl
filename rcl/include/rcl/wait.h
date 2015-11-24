// Copyright 2015 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RCL__WAIT_H_
#define RCL__WAIT_H_

#if __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>

#include "rcl/subscription.h"
#include "rcl/guard_condition.h"
#include "rcl/time.h"
#include "rcl/types.h"

struct rcl_wait_set_impl_t;

/// Container for subscription's, guard condition's, etc to be waited on.
typedef struct rcl_wait_set_t {
  /// Storage for subscription pointers.
  rcl_subscription_t ** subscriptions;
  size_t size_of_subscriptions;
  size_t __current_subscription_offset;
  /// Storage for guard condition pointers.
  rcl_guard_condition_t ** guard_conditions;
  size_t size_of_guard_conditions;
  size_t __current_guard_condition_offset;
  /// Allocator for storage.
  rcl_allocator_t allocator;
  /// If set to true, actions like add_subscription will fail until cleared.
  bool pruned;
  /// Implementation specific storage.
  rcl_wait_set_impl_t * impl;
} rcl_wait_set_t;

/// Return a rcl_wait_set_t struct with members set to NULL.
rcl_wait_set_t
rcl_get_zero_initialized_wait_set();

/// Initialize a rcl wait set with space for items to be waited on.
/* This function allocates space for the subscriptions and other wait-able
 * entities that can be stored in the wait set.
 * It also sets the allocator to the given one and initializes the pruned
 * member to be false.
 *
 * The wait_set struct should be allocated and initialized to NULL.
 * If the wait_set is allocated but the memory is uninitialized the behavior is
 * undefined.
 * Calling this function on a wait set that has already been initialized will
 * result in an error.
 * A wait set can be reinitialized if rcl_wait_set_fini() was called on it.
 *
 * To use the default allocator use rcl_get_default_allocator().
 *
 * Expected usage:
 *
 *   #include <rcl/wait.h>
 *
 *   rcl_wait_set_t wait_set = rcl_get_zero_initialized_wait_set();
 *   rcl_ret_t ret = rcl_wait_set_init(&wait_set, 42, 42, rcl_get_default_allocator());
 *   // ... error handling, and then after using call matching fini:
 *   ret = rcl_wait_set_fini(&wait_set);
 *   // ... error handling
 *
 * This function is thread-safe for different wait_set objects.
 * Thread-safety of this function requires a thread-safe allocator if the
 * allocator is shared with other parts of the system.
 *
 * \param[inout] wait_set the wait set struct to be initialized
 * \param[in] number_of_subscriptions size of the subscriptions set
 * \param[in] number_of_guard_conditions size of the guard conditions set
 * \param[in] allocator the allocator to use when allocating space in the sets
 * \return RCL_RET_OK if the wait set is initialized successfully, or
 *         RCL_RET_ALREADY_INIT if the wait set is not zero initialized, or
 *         RCL_RET_ERROR if an unspecified error occurs.
 */
rcl_ret_t
rcl_wait_set_init(
  rcl_wait_set_t * wait_set,
  size_t number_of_subscriptions,
  size_t number_of_guard_conditions,
  rcl_allocator_t allocator);

/// Finalize a rcl wait set.
/* Deallocates any memory in the wait set that was allocated in
 * rcl_wait_set_init() using the allocator given in the initialization.
 *
 * Calling this function on a zero initialized wait set will do nothing and
 * return RCL_RET_OK.
 * Calling this function on uninitialized memory results in undefined behavior.
 * After calling this function the wait set will once again be zero initialized
 * and so calling this function or rcl_wait_set_init() immediately after will
 * succeed.
 *
 * This function is not thread-safe.
 *
 * \param[inout] wait_set the wait set struct to be finalized.
 * \return RCL_RET_OK if the finalization was successful, otherwise RCL_RET_ERROR
 */
rcl_ret_t
rcl_wait_set_fini(rcl_wait_set_t * wait_set);

/// Stores a pointer to the given subscription in next empty spot in the set.
/* This function does not guarantee that the subscription is not already in the
 * wait set.
 *
 * This function is not thread-safe.
 *
 * \param[inout] wait_set struct in which the subscription is to be stored
 * \param[in] subscription the subscription to be added to the wait set
 * \return RCL_RET_OK if added successfully, or
 *         RCL_RET_WAIT_SET_FULL if the subscription set is full, or
 *         RCL_RET_NOT_INIT if the wait set is zero initialized, or
 *         RCL_RET_ERROR if an unspecified error occurs.
 */
rcl_ret_t
rcl_wait_set_add_subscription(
  rcl_wait_set_t * wait_set,
  const rcl_subscription_t * subscription);

/// Removes (sets to NULL) the subscriptions in the wait set.
/* This function should be used after passing using rcl_wait, but before
 * adding new subscriptions to the set.
 *
 * Calling this on an uninitialized (zero initialized) wait set will fail.
 *
 * This function is not thread-safe.
 *
 * \param[inout] wait_set struct to have its subscriptions cleared
 * \return RCL_RET_OK if cleared successfully, or
 *         RCL_RET_NOT_INIT if the wait set is zero initialized, or
 *         RCL_RET_ERROR if an unspecified error occurs.
 */
rcl_ret_t
rcl_wait_set_clear_subscriptions(rcl_wait_set_t * wait_set);

/// Reallocates space for the subscriptions in the wait set.
/* This function will deallocate and reallocate the memory for the
 * subscriptions set.
 *
 * A size of 0 will just deallocate the memory and assign NULL to the array.
 *
 * Allocation and deallocation is done with the allocator given during the
 * wait set's initialization.
 *
 * After calling this function all values in the set will be set to NULL,
 * effectively the same as calling rcl_wait_set_clear_subscriptions().
 *
 * If the requested size matches the current size, no allocation will be done.
 *
 * This can be called on an uninitialized (zero initialized) wait set.
 *
 * This function is not thread-safe.
 *
 * \param[inout] wait_set struct to have its subscriptions cleared
 * \param[in] size a size for the new set
 * \return RCL_RET_OK if resized successfully, or
 *         RCL_RET_ERROR if an unspecified error occurs.
 */
rcl_ret_t
rcl_wait_set_resize_subscriptions(rcl_wait_set_t * wait_set, size_t size);

/// Stores a pointer to the guard condition in next empty spot in the set.
/* This function behaves exactly the same as for subscriptions.
 * \see rcl_wait_set_add_subscription
 */
rcl_ret_t
rcl_wait_set_add_guard_condition(
  rcl_wait_set_t * wait_set,
  const rcl_guard_condition_t * guard_condition);

/// Removes (sets to NULL) the guard conditions in the wait set.
/* This function behaves exactly the same as for subscriptions.
 * \see rcl_wait_set_clear_subscriptions
 */
rcl_ret_t
rcl_wait_set_clear_guard_conditions(rcl_wait_set_t * wait_set);

/// Reallocates space for the subscriptions in the wait set.
/* This function behaves exactly the same as for subscriptions.
 * \see rcl_wait_set_resize_subscriptions
 */
rcl_ret_t
rcl_wait_set_resize_guard_conditions(rcl_wait_set_t * wait_set, size_t size);

/// Block until the wait set is ready or until the timeout has been exceeded.
/* This function will collect the items in the rcl_wait_set_t and pass them
 * to the underlying rmw_wait function.
 *
 * The items in the wait set will be either left untouched or set to NULL after
 * this function returns.
 * Items that are not NULL are ready, where ready means different things based
 * on the type of the item.
 * For subscriptions this means there are messages that can be taken.
 * For guard conditions this means the guard condition was triggered.
 *
 * Expected usage:
 *
 *   #include <rcl/rcl.h>
 *
 *   // rcl_init() called successfully before here...
 *   rcl_node_t node;  // initialize this, see rcl_node_init()
 *   rcl_subscription_t sub1;  // initialize this, see rcl_subscription_init()
 *   rcl_subscription_t sub2;  // initialize this, see rcl_subscription_init()
 *   rcl_guard_condition_t gc1;  // initialize this, see rcl_guard_condition_init()
 *   rcl_wait_set_t wait_set = rcl_get_zero_initialized_wait_set();
 *   rcl_ret_t ret = rcl_wait_set_init(&wait_set, 2, 1, rcl_get_default_allocator());
 *   // ... error handling
 *   do {
 *     ret = rcl_wait_set_clear_subscriptions(&wait_set);
 *     // ... error handling
 *     ret = rcl_wait_set_clear_guard_conditions(&wait_set);
 *     // ... error handling
 *     ret = rcl_wait_set_add_subscription(&wait_set, &sub1);
 *     // ... error handling
 *     ret = rcl_wait_set_add_subscription(&wait_set, &sub2);
 *     // ... error handling
 *     ret = rcl_wait_set_add_guard_condition(&wait_set, &gc1);
 *     // ... error handling
 *     rcl_time_t timeout = {1, 0};  // 1 second and 0 nanoseconds.
 *     ret = rcl_wait(&wait_set, &timeout);
 *     if (ret == RCL_RET_TIMEOUT) {
 *       continue;
 *     }
 *     for (int i = 0; i < wait_set.size_of_subscriptions; ++i) {
 *       if (wait_set.subscriptions[i]) {
 *         // The subscription is ready...
 *       }
 *     }
 *     for (int i = 0; i < wait_set.size_of_guard_conditions; ++i) {
 *       if (wait_set.guard_conditions[i]) {
 *         // The subscription is ready...
 *       }
 *     }
 *   } while(check_some_condition());
 *   // ... fini node, and subscriptions and guard conditions...
 *   ret = rcl_wait_set_fini(&wait_set);
 *   // ... error handling
 *
 * The wait set struct must be allocated, initialized, and should have been
 * cleared and then filled with items, e.g. subscriptions and guard conditions.
 * Passing a wait set with no wait-able items in it will fail.
 * NULL items in the sets are ignored, e.g. it is valid to have as input:
 *  - subscriptions[0] = valid pointer
 *  - subscriptions[1] = NULL
 *  - subscriptions[2] = valid pointer
 *  - size_of_subscriptions = 3
 * Passing an uninitialized (zero initialized) wait set struct will fail.
 * Passing a wait set struct with uninitialized memory is undefined behavior.
 *
 * If the timeout pointer is NULL then this function will block indefinitely
 * until something in the wait set is valid or it is interrupted.
 * If the timeout contains 0 for seconds and nanoseconds this function will be
 * non-blocking; checking what's ready but not waiting if nothing is ready yet.
 * If the timeout contains a non-zero time then this function will return after
 * that period of time has elapsed if something in the wait set does not become
 * ready before then.
 * The timeout's value is not changed.
 * Passing a timeout struct with uninitialized memory is undefined behavior.
 *
 * This function is not thread-safe and cannot be called concurrently, even if
 * the given wait sets are not the same and non-overlapping in contents.
 *
 * \param[inout] wait_set the set of things to be waited on and to be pruned if not ready
 * \param[in] timeout the time duration to wait something in the wait set to be ready
 * \return RCL_RET_OK something in the wait set became ready, or
 *         RCL_RET_TIMEOUT timeout expired before something was ready, or
 *         RCL_RET_NOT_INIT wait set is zero initialized, or
 *         RCL_RET_WAIT_SET_EMPTY wait set contains no items, or
 *         RCL_RET_ERROR an unspecified error occur.
 */
rcl_ret_t
rcl_wait(rcl_wait_set_t * wait_set, const rcl_time_t * timeout);

#if __cplusplus
}
#endif

#endif  // RCL__WAIT_H_
