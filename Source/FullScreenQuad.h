#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"

struct Vertex {
    glm::vec2 position;
    glm::vec2 uv;
};

struct GBuffer {
    ImageData Position;
    ImageData Normal;
    ImageData Albedo;
};

struct LightUniformData {
     glm::vec4  lightPositionAndLightType;
     glm::vec4  colorAndAmbientStrength;
     glm::vec4  CameraPositionAndLightIntensity;
};
class FullScreenQuad : public Drawable
{
public:

    FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool);
    void CreateVertexAndIndexBuffer() override;
    void CreateUniformBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer Gbuffer) ;
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref);
    //void UpdateUniformBuffer(uint32_t currentImage, Light* lightref) override;
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void CleanUp() ;
   // LightUniformData lightData;

   private:

        std::vector<Vertex> quad = {
           {{-1.0f, -1.0f}, {0.0f, 0.0f}}, // Bottom-left
           {{ 1.0f, -1.0f}, {1.0f, 0.0f}}, // Bottom-right
           {{-1.0f,  1.0f}, {0.0f, 1.0f}}, // Top-left
           {{ 1.0f,  1.0f}, {1.0f, 1.0f}}  // Top-right
       };

        const std::vector<uint16_t> quadIndices = {
               0, 1, 2,
               2, 1, 3 
        };


        Camera* camera = nullptr;
};

static inline void FullScreenQuadDeleter(FullScreenQuad* fullScreenQuad) {
    if (fullScreenQuad)
    {
        fullScreenQuad->CleanUp();
        delete fullScreenQuad;
    }
}

