#include "renderupload/FrameUploadScheduler.h"

#include <algorithm>

namespace RenderUpload {

FrameUploadScheduler::FrameUploadScheduler()
    : nextHandle_(1), nextSequence_(1) {}

UploadHandle FrameUploadScheduler::Enqueue(const UploadRequest& request) {
    PendingRequest pending;
    pending.request = request;
    pending.sequence = nextSequence_++;
    pending.request.handle = UploadHandle(nextHandle_++);
    if (pending.request.byteSize == 0 && pending.request.HasPayload()) {
        pending.request.byteSize = static_cast<std::uint64_t>(pending.request.PayloadSize());
    }
    pending_.push_back(pending);
    return pending.request.handle;
}

void FrameUploadScheduler::MarkCompleted(UploadHandle handle) {
    if (handle.IsValid()) {
        completed_.insert(handle.id);
    }
}

bool FrameUploadScheduler::IsCompleted(UploadHandle handle) const {
    return handle.IsValid() && completed_.find(handle.id) != completed_.end();
}

std::size_t FrameUploadScheduler::GetPendingCount() const {
    return pending_.size();
}

bool FrameUploadScheduler::IsBufferTask(UploadTaskType type) {
    return type == UploadTaskType::VertexBuffer || type == UploadTaskType::IndexBuffer;
}

bool FrameUploadScheduler::HasPayload(const UploadRequest& request) {
    return request.byteSize > 0 || request.HasPayload();
}

bool FrameUploadScheduler::CanFitTask(const UploadRequest& request,
                                      const UploadBudget& budget,
                                      std::uint32_t selectedTaskCount,
                                      std::uint64_t selectedBytes,
                                      std::uint32_t selectedTextureCount,
                                      std::uint32_t selectedBufferCount) {
    if (budget.maxTaskCount == 0) {
        return false;
    }
    if (selectedTaskCount >= budget.maxTaskCount) {
        return false;
    }
    if (request.type == UploadTaskType::Texture2D && selectedTextureCount >= budget.maxTextureCount) {
        return false;
    }
    if (IsBufferTask(request.type) && selectedBufferCount >= budget.maxBufferCount) {
        return false;
    }
    if (HasPayload(request) && budget.maxBytes == 0) {
        return false;
    }

    const std::uint64_t taskBytes = request.byteSize;
    if (taskBytes > 0 && selectedBytes + taskBytes > budget.maxBytes) {
        return selectedTaskCount == 0;
    }
    return true;
}

std::uint32_t FrameUploadScheduler::SubmitFrame(const UploadBudget& budget, const SubmitCallback& submit) {
    return SubmitFrameInternal(0, false, budget, submit);
}

std::uint32_t FrameUploadScheduler::SubmitFrameForOwner(std::uint64_t ownerId,
                                                        const UploadBudget& budget,
                                                        const SubmitCallback& submit) {
    return SubmitFrameInternal(ownerId, true, budget, submit);
}

std::uint32_t FrameUploadScheduler::SubmitFrameInternal(std::uint64_t ownerId,
                                                        bool filterOwner,
                                                        const UploadBudget& budget,
                                                        const SubmitCallback& submit) {
    if (!submit || pending_.empty() || budget.maxTaskCount == 0) {
        return 0;
    }

    std::stable_sort(pending_.begin(), pending_.end(), [](const PendingRequest& lhs, const PendingRequest& rhs) {
        const int lp = static_cast<int>(lhs.request.priority);
        const int rp = static_cast<int>(rhs.request.priority);
        if (lp != rp) {
            return lp > rp;
        }
        return lhs.sequence < rhs.sequence;
    });

    std::vector<PendingRequest> remaining;
    remaining.reserve(pending_.size());

    std::uint32_t submitted = 0;
    std::uint64_t submittedBytes = 0;
    std::uint32_t submittedTextures = 0;
    std::uint32_t submittedBuffers = 0;

    for (std::size_t i = 0; i < pending_.size(); ++i) {
        const PendingRequest& pending = pending_[i];
        const UploadRequest& request = pending.request;
        if (filterOwner && request.ownerId != ownerId) {
            remaining.push_back(pending);
            continue;
        }

        if (CanFitTask(request, budget, submitted, submittedBytes, submittedTextures, submittedBuffers)) {
            submit(request);
            ++submitted;
            submittedBytes += request.byteSize;
            if (request.type == UploadTaskType::Texture2D) {
                ++submittedTextures;
            } else if (IsBufferTask(request.type)) {
                ++submittedBuffers;
            }
        } else {
            remaining.push_back(pending);
        }
    }

    pending_.swap(remaining);
    return submitted;
}

} // namespace RenderUpload
