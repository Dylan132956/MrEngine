#include "asset/image/ImageDecodeBatch.h"

#include <cstdio>

namespace ImagePipeline {

ImageDecodeBatch::ImageDecodeBatch()
    : nextSubmitIndex_(0), hasFailures_(false) {}

void ImageDecodeBatch::Reset(const std::vector<ImageSource>& sources, const ImageDecodeBatchDesc& desc) {
    desc_ = desc;
    sources_ = sources;
    pending_.clear();
    decodedImages_.assign(sources_.size(), std::shared_ptr<ImageDecode::DecodedImageAsset>());
    nextSubmitIndex_ = 0;
    hasFailures_ = false;
}

std::uint32_t ImageDecodeBatch::Submit(AsyncLoad::AssetManager& assetManager) {
    std::uint32_t submitted = 0;
    std::uint64_t submittedBytes = 0;

    while (nextSubmitIndex_ < sources_.size()) {
        if (submitted >= desc_.maxSubmitCountPerFrame) {
            break;
        }

        const std::size_t i = nextSubmitIndex_;
        const ImageSource& source = sources_[i];
        const std::size_t byteSize = source.BytesSize();
        if (submitted > 0 && submittedBytes + static_cast<std::uint64_t>(byteSize) > desc_.maxSubmitBytesPerFrame) {
            break;
        }

        const std::string requestId = source.id.empty() ? std::string("image/") + std::to_string(i) : source.id;
        AsyncLoad::AssetLoadRequest request = source.sharedBytes
            ? AsyncLoad::AssetLoadRequest::FromSharedMemory(requestId, source.sharedBytes, source.mimeType)
            : AsyncLoad::AssetLoadRequest::FromMemory(requestId, source.encodedBytes, source.mimeType);
        request.debugName = source.name.empty() ? requestId : source.name;

        PendingImageDecode pending;
        pending.handle = assetManager.LoadAsync<ImageDecode::DecodedImageAsset>(request);
        pending.imageIndex = i;
        pending_.push_back(pending);

        submittedBytes += static_cast<std::uint64_t>(byteSize);
        ++submitted;
        ++nextSubmitIndex_;
    }

    return submitted;
}

void ImageDecodeBatch::Poll() {
    if (pending_.empty()) {
        return;
    }

    std::uint32_t processed = 0;
    std::vector<PendingImageDecode> remaining;
    remaining.reserve(pending_.size());

    for (std::size_t i = 0; i < pending_.size(); ++i) {
        PendingImageDecode& pending = pending_[i];
        if (!pending.handle.IsDone() || processed >= desc_.maxPollCountPerFrame) {
            remaining.push_back(pending);
            continue;
        }

        ++processed;
        if (pending.handle.IsSucceeded()) {
            std::shared_ptr<ImageDecode::DecodedImageAsset> image = pending.handle.Get();
            if (pending.imageIndex < decodedImages_.size()) {
                decodedImages_[pending.imageIndex] = image;
            }
        } else {
            hasFailures_ = true;
            std::printf("[ImageDecodeBatch] Image decode failed index=%zu error=%s\n",
                pending.imageIndex,
                pending.handle.GetError().c_str());
        }
    }

    pending_.swap(remaining);
}

void ImageDecodeBatch::Cancel() {
    for (std::size_t i = 0; i < pending_.size(); ++i) {
        pending_[i].handle.Cancel();
    }
}

bool ImageDecodeBatch::IsSubmitComplete() const {
    return nextSubmitIndex_ >= sources_.size();
}

bool ImageDecodeBatch::IsDone() const {
    return IsSubmitComplete() && pending_.empty();
}

bool ImageDecodeBatch::HasFailures() const {
    return hasFailures_;
}

const std::vector<std::shared_ptr<ImageDecode::DecodedImageAsset> >& ImageDecodeBatch::GetDecodedImages() const {
    return decodedImages_;
}

} // namespace ImagePipeline
