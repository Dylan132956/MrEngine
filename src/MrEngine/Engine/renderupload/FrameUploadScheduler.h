#pragma once

#include "renderupload/FrameUploadTypes.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <vector>

namespace RenderUpload {

class FrameUploadScheduler {
public:
    typedef std::function<void(const UploadRequest&)> SubmitCallback;

    FrameUploadScheduler();

    UploadHandle Enqueue(const UploadRequest& request);
    std::uint32_t SubmitFrame(const UploadBudget& budget, const SubmitCallback& submit);
    std::uint32_t SubmitFrameForOwner(std::uint64_t ownerId, const UploadBudget& budget, const SubmitCallback& submit);

    void MarkCompleted(UploadHandle handle);
    bool IsCompleted(UploadHandle handle) const;
    std::size_t GetPendingCount() const;

private:
    struct PendingRequest {
        UploadRequest request;
        std::uint64_t sequence;
    };

    static bool IsBufferTask(UploadTaskType type);
    static bool HasPayload(const UploadRequest& request);
    static bool CanFitTask(const UploadRequest& request,
                           const UploadBudget& budget,
                           std::uint32_t selectedTaskCount,
                           std::uint64_t selectedBytes,
                           std::uint32_t selectedTextureCount,
                           std::uint32_t selectedBufferCount);

    std::uint32_t SubmitFrameInternal(std::uint64_t ownerId,
                                      bool filterOwner,
                                      const UploadBudget& budget,
                                      const SubmitCallback& submit);

    std::uint64_t nextHandle_;
    std::uint64_t nextSequence_;
    std::vector<PendingRequest> pending_;
    std::set<std::uint64_t> completed_;
};

} // namespace RenderUpload
