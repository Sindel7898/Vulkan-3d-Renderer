#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Light.h"
#include "Drawable.h"
#include "structs.h"


class Terrain : public Drawable
{
public:

    Terrain(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void LoadTextures();
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void CreateUniformBuffer() override;
    void UpdateUniformBuffer(uint32_t currentImage);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CleanUp() ;


    ImageData  albedoTextureData;
    ImageData  normalTextureData;
    ImageData  MetallicRoughnessTextureData;
    ImageData  HeighMapTextureData;

    std::vector<ModelVertex> VertexData;

private:

    std::string FilePath;

    const StoredModelData* storedModelData = nullptr;


    BufferData bottomLevelASBuffer;
    BufferData scratchBuffer;
};


static inline void TerrainDeleter(Terrain* terrain) {

        if (terrain) {
            terrain->CleanUp();
            delete terrain;
        }
   
};