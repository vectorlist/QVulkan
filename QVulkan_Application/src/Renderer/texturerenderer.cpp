#include "TextureRenderer.h"
#include <qwindow.h>
#include <vktools.h>
#include <vkinitializer.h>
#include <shader.h>
#include <scene.h>
#include <mesh.h>
#include <vertex.h>
#include <vkswapchain.h>
#include <texture.h>

TextureRenderer::TextureRenderer(QWindow* window)
	: VkRenderer(window)//, //m_scene(NULL)
{
	//enabledFeatures
}


TextureRenderer::~TextureRenderer()
{
	SAFE_DELETE(m_texture);
	SAFE_DELETE(m_scene);

	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	m_pipelineLayout = VK_NULL_HANDLE;
	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
}

void TextureRenderer::buildProcedural()
{
	VkRenderer::initialize();
	VkRenderer::buildProcedural();
	buildScene();
	buildTexture();

	buildDescriptorSetLayout();
	buildPipeline();
	buildDescriptorPool();
	buildDescriptorSet();
	buildCommandBuffers();

	isBuilt = true;
}

void TextureRenderer::buildScene()
{
	LOG_SECTION("build scene");
	m_scene = new Scene(this);

	mesh_ptr mesh = mesh_ptr(new MeshObject);
	meshTool::LoadModel("./model/sphinxfixed.obj", mesh.get());

	shader_ptr shader = shader_ptr(new Shader(m_device));
	//SPV
	shader->buildSPV("./shader/texturerenderer/vert.spv", "./shader/texturerenderer/frag.spv");
	//shader->buildGLSL("./shader/texturerenderer/shader.vert", "./shader/texturerenderer/shader.frag");
	//shader->loadShaderGLSL("./shader/texturerenderer/shader.vert",m_device,
	//VK_SHADER_STAGE_VERTEX_BIT);

	//have  a look
	VkShaderModule testModule = shaderTool::loadShaderGLSL("./shader/texturerenderer/triangle.vert", m_device,
		VK_SHADER_STAGE_VERTEX_BIT);

	camera_ptr camera = camera_ptr(new Camera);

	m_scene->addElement(camera);
	m_scene->addElement(mesh);
	m_scene->addElement(shader);

	m_scene->buildVertexBuffer();
	m_scene->buildIndiceBuffer();
	m_scene->initUniformBuffer();
	m_scene->buildInputState();

	m_scene->updateUnifomrBuffers();
}


void TextureRenderer::buildDescriptorSetLayout()
{
	LOG_SECTION("create descriptor set layout and pipeline layout");
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		uboLayoutBinding, samplerLayoutBinding
	};
	/*CREATE DESCRIPTOR LAYOUT*/
	VkDescriptorSetLayoutCreateInfo descritorSetlayoutInfo{};
	descritorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descritorSetlayoutInfo.bindingCount = bindings.size();
	descritorSetlayoutInfo.pBindings = bindings.data();

	LOG_ERROR("failed to create descriptor set layouts") <<
		vkCreateDescriptorSetLayout(m_device, &descritorSetlayoutInfo, nullptr, &m_descriptorSetLayout);

	/*CREATE PIPELINE LAYOUT*/
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = NULL;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

	LOG_ERROR("failed to create pipeline layout") <<
		vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
}


void TextureRenderer::buildPipeline()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = defaultTopology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)width;
	viewport.height = (float)height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { width, height };

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterState{};
	rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterState.depthClampEnable = VK_FALSE;
	rasterState.rasterizerDiscardEnable = VK_FALSE;
	rasterState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterState.lineWidth = 1.0f;
	rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //draw for right hand maybe
	rasterState.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multiSamplingState{};
	multiSamplingState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSamplingState.sampleShadingEnable = VK_FALSE;
	multiSamplingState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	//depth
	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; //same as GL_DEPTH_LESS
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;	//same as GL_ENABLE_DEPTH_TEST

	VkPipelineColorBlendAttachmentState colorBlendattachment{};
	colorBlendattachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendattachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendattachment;
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;


	std::vector<VkDynamicState> dynamicStateEnables{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();


	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	//pipelineInfo.pVertexInputState = &vertexInputinfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterState;
	pipelineInfo.pMultisampleState = &multiSamplingState;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	/*VERTEX INPUT STATE FROM SCENE*/
	pipelineInfo.pVertexInputState = &m_scene->vertexInputState;

	/*SHADER STAGE FROM SCENE*/
	pipelineInfo.stageCount = m_scene->shaders[0]->shaderStage.size();
	pipelineInfo.pStages = m_scene->shaders[0]->shaderStage.data();

	for(auto &mesh : m_scene->meshs )
	{
		LOG_ERROR("failed to create mesh graphic pipeline") <<
		vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr,
				&mesh->pipeline);
	}

	vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr,
		&m_scene->wireframePipeline);
	

}

void TextureRenderer::buildDescriptorPool()
{
	LOG_SECTION("create descriptor pool");
	std::array<VkDescriptorPoolSize, 2> poolSize = {};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 1;
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSize.size();
	poolInfo.pPoolSizes = poolSize.data();
	poolInfo.maxSets = 1;
	LOG << "descriptor pool size" << poolSize.size() << ENDL;

	LOG_ERROR("failed to create descriptor pool") <<
		vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
}

void TextureRenderer::buildDescriptorSet()
{
	LOG_SECTION("update descriptor set");
	VkDescriptorSetLayout layout[] = { m_descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocateinfo{};
	allocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateinfo.descriptorPool = m_descriptorPool;
	allocateinfo.descriptorSetCount = 1;
	allocateinfo.pSetLayouts = layout;

	LOG_ERROR("failed to allocate descriptor set") <<
	vkAllocateDescriptorSets(m_device, &allocateinfo, &m_descriptorSet);

	VkDescriptorImageInfo imageInfo{};

	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = NULL;				//null for atm
	imageInfo.sampler = NULL;				//yet implement

	VkDescriptorBufferInfo bufferinfo{};
	bufferinfo.buffer = m_scene->ubo.buffer;
	bufferinfo.offset = 0;
	bufferinfo.range = sizeof(UBODataType);

	VkWriteDescriptorSet uniformWrites{};
	//vertex shader binding 0
	uniformWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniformWrites.dstSet = m_descriptorSet;
	uniformWrites.dstBinding = 0;
	uniformWrites.dstArrayElement = 0;
	uniformWrites.descriptorCount = 1;
	uniformWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformWrites.pBufferInfo = &bufferinfo;

	VkWriteDescriptorSet imageWrites{};
	//frag shader binding 1
	imageWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	imageWrites.dstSet = m_descriptorSet;
	imageWrites.dstBinding = 1;
	imageWrites.dstArrayElement = 0;
	imageWrites.descriptorCount = 1;
	imageWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageWrites.pImageInfo = &m_texture->descriptor;

	/*std::vector<VkWriteDescriptorSet> writesDescriptors{
		uniformWrites, imageWrites
	};*/
	std::vector<VkWriteDescriptorSet> writeDescriptors;
	writeDescriptors.push_back(uniformWrites);
	writeDescriptors.push_back(imageWrites);
	vkUpdateDescriptorSets(m_device, writeDescriptors.size(), writeDescriptors.data(),
		0, nullptr);
	/*vkUpdateDescriptorSets(m_device, 1, &descriptorWrites,
		0, nullptr);*/

}

void TextureRenderer::buildCommandBuffers()
{
	count++;
	VkCommandBufferBeginInfo cmdBufInfo = vkInitializer::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = defaultClearColor;
	clearValues[1].depthStencil = { 1.0f,0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vkInitializer::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent = { width,height };
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (uint32_t i = 0; i < m_commandBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = m_frameBuffers[i];

		vkBeginCommandBuffer(m_commandBuffers[i], &cmdBufInfo);
		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkInitializer::viewport((float)width, (float)height,
			0.0f, 1.0f);
		vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkInitializer::rect2D(width, height, 0, 0);
		vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout, 0, 1, &m_descriptorSet, 0, NULL);

		for (auto& mesh : m_scene->meshs)
		{
			mesh->render(m_commandBuffers[i]);
		}

		vkCmdEndRenderPass(m_commandBuffers[i]);

		vkEndCommandBuffer(m_commandBuffers[i]);
	}

}

void TextureRenderer::buildTexture()
{
	m_texture = new Texture(m_vulkanDevice);
	//m_texture->loadTexture("./image/checker.jpg", VK_FORMAT_BC2_UNORM_BLOCK, false);
	m_texture->loadTexture("./image/sphinx01.jpg", VK_FORMAT_R8G8B8A8_UNORM, false);
}

void TextureRenderer::render()
{
	VkRenderer::begin();

	m_submitInfo.commandBufferCount = 1;
	m_submitInfo.pCommandBuffers = &m_commandBuffers[m_currentBuffer];

	vkQueueSubmit(m_queue, 1, &m_submitInfo, VK_NULL_HANDLE);

	VkRenderer::end();
}

void TextureRenderer::updateUniformBuffers()
{
	m_scene->updateUnifomrBuffers();
}