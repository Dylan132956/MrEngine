/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vulkan/VulkanDisposer.h"
#include <assert.h>

namespace filament {
namespace backend {

void VulkanDisposer::createDisposable(Key resource, std::function<void()> destructor) noexcept {
    mDisposables[resource].destructor = destructor;
}

void VulkanDisposer::addReference(Key resource) noexcept {
	assert(mDisposables.count(resource) > 0);
	auto& ptr = mDisposables[resource];
	assert(ptr.refcount > 0);

    ++ptr.refcount;
}

void VulkanDisposer::removeReference(Key resource) noexcept {
	assert(mDisposables.count(resource) > 0);
	auto& ptr = mDisposables[resource];
	assert(ptr.refcount > 0);

	--ptr.refcount;
    if (ptr.refcount == 0) {
        mGraveyard.emplace_back(std::move(ptr));
        mDisposables.erase(resource);
    }
}

void VulkanDisposer::acquire(Key resource, Set& resources) noexcept {
    auto iter = resources.find(resource);
    if (iter == resources.end()) {
        resources.insert(resource);
        addReference(resource);
    }
}

void VulkanDisposer::release(Set& resources) {
    for (auto resource : resources) {
        removeReference(resource);
    }
    resources.clear();
}

void VulkanDisposer::gc() noexcept {
    for (auto& ptr : mGraveyard) {
        ptr.destructor();
    }
    mGraveyard.clear();
}

void VulkanDisposer::reset() noexcept {
    gc();
    assert(mDisposables.empty());
}

} // namespace filament
} // namespace backend
