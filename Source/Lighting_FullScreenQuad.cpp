#include "Lighting_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"

Lighting_FullScreenQuad::Lighting_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref): Drawable()
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	ReflectiveCubeMapData = &skyboxref->SkyBoxTextureData;

	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}


void Lighting_FullScreenQuad::CreateVertexAndIndexBuffer()
{

	VkDeviceSize VertexBufferSize = sizeof(quad[0]) * quad.size();
	vertexBufferData.BufferID = " Lighting FullScreen Quad Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData, quad.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint16_t) * quadIndices.size();
	indexBufferData.BufferID = " Lighting FullScreen Quad Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,quadIndices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

void Lighting_FullScreenQuad::CreateUniformBuffer()
{
	//////////////////////////////////////////////////////////////
	fragmentUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	FragmentUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize FragmentuniformBufferSize = sizeof(LightUniformData) * 100;

	for (size_t i = 0; i < fragmentUniformBuffers.size(); i++)
	{
		BufferData bufferdata;

		bufferdata.BufferID = " Lighting FullScreen Quad Fragment Uniform Buffer" + i;
		bufferManager->CreateBuffer(&bufferdata,FragmentuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		fragmentUniformBuffers[i] = bufferdata;

		FragmentUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
	}
}

void Lighting_FullScreenQuad::createDescriptorSetLayout()
{
	{
		//////// Create set for Final Lighting Pass ////////////
		vk::DescriptorSetLayoutBinding PositionSampleryLayout{};
		PositionSampleryLayout.binding = 0;
		PositionSampleryLayout.descriptorCount = 1;
		PositionSampleryLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSampleryLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding NormalSamplerLayout{};
		NormalSamplerLayout.binding = 1;
		NormalSamplerLayout.descriptorCount = 1;
		NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding AlbedoSamplerLayout{};
		AlbedoSamplerLayout.binding = 2;
		AlbedoSamplerLayout.descriptorCount = 1;
		AlbedoSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding SSAOSamplerLayout{};
		SSAOSamplerLayout.binding = 3;
		SSAOSamplerLayout.descriptorCount = 1;
		SSAOSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSAOSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding MaterialsSamplerLayout{};
		MaterialsSamplerLayout.binding = 4;
		MaterialsSamplerLayout.descriptorCount = 1;
		MaterialsSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding ReflectiveCubeSamplerLayout{};
		ReflectiveCubeSamplerLayout.binding = 5;
		ReflectiveCubeSamplerLayout.descriptorCount = 1;
		ReflectiveCubeSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectiveCubeSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding ReflectionMaskSamplerLayout{};
		ReflectionMaskSamplerLayout.binding = 6;
		ReflectionMaskSamplerLayout.descriptorCount = 1;
		ReflectionMaskSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectionMaskSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding LightUniformBufferLayout{};
		LightUniformBufferLayout.binding = 7;
		LightUniformBufferLayout.descriptorCount = 1;
		LightUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;


		std::array<vk::DescriptorSetLayoutBinding, 8> bindings = { PositionSampleryLayout,        // binding 0
																   NormalSamplerLayout,           // binding 1
																   AlbedoSamplerLayout,           // binding 2
			                                                       SSAOSamplerLayout,
			                                                       MaterialsSamplerLayout,
																   ReflectiveCubeSamplerLayout,
																   ReflectionMaskSamplerLayout,
																   LightUniformBufferLayout       // binding 3
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}

}

void Lighting_FullScreenQuad::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer Gbuffer,ImageData ReflectionMask)
{
	// create sets from the pool based on the layout
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	////////////////////////////////////////////////////////////////////////////////////////////////
	//specifies what exactly to send
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		/////////////////////////////////////////////////////////////////////////////////////
		vk::DescriptorImageInfo PositionimageInfo{};
		PositionimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		PositionimageInfo.imageView = Gbuffer.Position.imageView;
		PositionimageInfo.sampler = Gbuffer.Position.imageSampler;

		vk::WriteDescriptorSet PositionSamplerdescriptorWrite{};
		PositionSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		PositionSamplerdescriptorWrite.dstBinding = 0;
		PositionSamplerdescriptorWrite.dstArrayElement = 0;
		PositionSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSamplerdescriptorWrite.descriptorCount = 1;
		PositionSamplerdescriptorWrite.pImageInfo = &PositionimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////
;
        vk::DescriptorImageInfo NormalimageInfo{};
        NormalimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        NormalimageInfo.imageView = Gbuffer.Normal.imageView;
        NormalimageInfo.sampler = Gbuffer.Normal.imageSampler;
        
        vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
        NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        NormalSamplerdescriptorWrite.dstBinding = 1;
        NormalSamplerdescriptorWrite.dstArrayElement = 0;
        NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        NormalSamplerdescriptorWrite.descriptorCount = 1;
        NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo AlbedoimageInfo{};
		AlbedoimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		AlbedoimageInfo.imageView = Gbuffer.Albedo.imageView;
		AlbedoimageInfo.sampler = Gbuffer.Albedo.imageSampler;

		vk::WriteDescriptorSet  AlbedoSamplerdescriptorWrite{};
		AlbedoSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		AlbedoSamplerdescriptorWrite.dstBinding = 2;
		AlbedoSamplerdescriptorWrite.dstArrayElement = 0;
		AlbedoSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerdescriptorWrite.descriptorCount = 1;
		AlbedoSamplerdescriptorWrite.pImageInfo = &AlbedoimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo SSAOimageInfo{};
		SSAOimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		SSAOimageInfo.imageView = Gbuffer.SSAOBlured.imageView;
		SSAOimageInfo.sampler = Gbuffer.SSAOBlured.imageSampler;

		vk::WriteDescriptorSet SSAOSamplerdescriptorWrite{};
		SSAOSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SSAOSamplerdescriptorWrite.dstBinding = 3;
		SSAOSamplerdescriptorWrite.dstArrayElement = 0;
		SSAOSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSAOSamplerdescriptorWrite.descriptorCount = 1;
		SSAOSamplerdescriptorWrite.pImageInfo = &SSAOimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo MaterialsimageInfo{};
		MaterialsimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		MaterialsimageInfo.imageView = Gbuffer.Materials.imageView;
		MaterialsimageInfo.sampler = Gbuffer.Materials.imageSampler;

		vk::WriteDescriptorSet MaterialsSamplerdescriptorWrite{};
		MaterialsSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		MaterialsSamplerdescriptorWrite.dstBinding = 4;
		MaterialsSamplerdescriptorWrite.dstArrayElement = 0;
		MaterialsSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerdescriptorWrite.descriptorCount = 1;
		MaterialsSamplerdescriptorWrite.pImageInfo = &MaterialsimageInfo;


		vk::DescriptorImageInfo ReflectiveCubeimageInfo{};
		ReflectiveCubeimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ReflectiveCubeimageInfo.imageView = ReflectiveCubeMapData->imageView;
		ReflectiveCubeimageInfo.sampler = ReflectiveCubeMapData->imageSampler;

		vk::WriteDescriptorSet ReflectiveCubeSamplerdescriptorWrite{};
		ReflectiveCubeSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		ReflectiveCubeSamplerdescriptorWrite.dstBinding = 5;
		ReflectiveCubeSamplerdescriptorWrite.dstArrayElement = 0;
		ReflectiveCubeSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectiveCubeSamplerdescriptorWrite.descriptorCount = 1;
		ReflectiveCubeSamplerdescriptorWrite.pImageInfo = &ReflectiveCubeimageInfo;


		vk::DescriptorImageInfo ReflectionMaskimageInfo{};
		ReflectionMaskimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ReflectionMaskimageInfo.imageView = ReflectionMask.imageView;
		ReflectionMaskimageInfo.sampler = ReflectionMask.imageSampler;

		vk::WriteDescriptorSet ReflectionMaskSamplerdescriptorWrite{};
		ReflectionMaskSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		ReflectionMaskSamplerdescriptorWrite.dstBinding = 6;
		ReflectionMaskSamplerdescriptorWrite.dstArrayElement = 0;
		ReflectionMaskSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectionMaskSamplerdescriptorWrite.descriptorCount = 1;
		ReflectionMaskSamplerdescriptorWrite.pImageInfo = &ReflectionMaskimageInfo;

		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorBufferInfo LightUniformBufferInfo;
		LightUniformBufferInfo.buffer = fragmentUniformBuffers[i].buffer;
		LightUniformBufferInfo.offset = 0;
		LightUniformBufferInfo.range = sizeof(LightUniformData) * 100;

		vk::WriteDescriptorSet LightUniformBufferDescriptorWrite{};
		LightUniformBufferDescriptorWrite.dstSet = DescriptorSets[i];
		LightUniformBufferDescriptorWrite.dstBinding = 7;
		LightUniformBufferDescriptorWrite.dstArrayElement = 0;
		LightUniformBufferDescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferDescriptorWrite.descriptorCount = 1;
		LightUniformBufferDescriptorWrite.pBufferInfo = &LightUniformBufferInfo;

		std::array<vk::WriteDescriptorSet, 8> descriptorWrites = {
	                                                                PositionSamplerdescriptorWrite,        // binding 0
	                                                                NormalSamplerdescriptorWrite,          // binding 1
	                                                                AlbedoSamplerdescriptorWrite,          // binding 2
																	SSAOSamplerdescriptorWrite,
																	MaterialsSamplerdescriptorWrite,
																	ReflectiveCubeSamplerdescriptorWrite,
																	ReflectionMaskSamplerdescriptorWrite,
	                                                                LightUniformBufferDescriptorWrite      // binding 3
		                                                         };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}




}

void Lighting_FullScreenQuad::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref)
{
	std::vector<LightUniformData> lightDataspack;
	lightDataspack.reserve(lightref.size());

	for (int  i = 0; i < lightref.size(); i++)
	{
		Drawable::UpdateUniformBuffer(currentImage);

		if (lightref[i])
		{
			LightUniformData LightData;
			LightData.lightPositionAndLightType = glm::vec4(lightref[i]->position,lightref[i]->lightType);
			LightData.colorAndAmbientStrength  = glm::vec4(lightref[i]->color, lightref[i]->ambientStrength);
			LightData.CameraPositionAndLightIntensity = glm::vec4(camera->GetPosition().x, 
				                                                  camera->GetPosition().y,
				                                                  camera->GetPosition().z, 
				                                                  lightref[i]->lightIntensity);

			glm::mat4 projection = lightref[i]->ProjectionMatrix;
			projection[1][1] *= -1;
			LightData.LightViewProjMatrix = projection * lightref[i]->ViewMatrix;
			lightDataspack.push_back(LightData);
		}
	}

	memcpy(FragmentUniformBuffersMappedMem[currentImage], lightDataspack.data(), lightDataspack.size() * sizeof(LightUniformData));

}

void Lighting_FullScreenQuad::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void Lighting_FullScreenQuad::CleanUp()
{

	Drawable::Destructor();
}

