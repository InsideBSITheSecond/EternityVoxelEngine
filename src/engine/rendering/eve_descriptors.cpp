#include "eve_descriptors.hpp"

// std
#include <cassert>
#include <stdexcept>

namespace eve
{

	// *************** Descriptor Set Layout Builder *********************

	EveDescriptorSetLayout::Builder &EveDescriptorSetLayout::Builder::addBinding(
		uint32_t binding,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		uint32_t count)
	{
		assert(bindings.count(binding) == 0 && "Binding already in use");
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;
		bindings[binding] = layoutBinding;
		return *this;
	}

	std::unique_ptr<EveDescriptorSetLayout> EveDescriptorSetLayout::Builder::build(std::vector<VkDescriptorBindingFlags> flags) const
	{
		return std::make_unique<EveDescriptorSetLayout>(eveDevice, bindings, flags);
	}

	// *************** Descriptor Set Layout *********************

	EveDescriptorSetLayout::EveDescriptorSetLayout(
		EveDevice &eveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings, std::vector<VkDescriptorBindingFlags> flags)
		: eveDevice{eveDevice}, bindings{bindings}, flags{flags}
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
		for (auto kv : bindings)
		{
			setLayoutBindings.push_back(kv.second);
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
		bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlagsInfo.bindingCount = setLayoutBindings.size();
		bindingFlagsInfo.pBindingFlags = flags.data();

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();
		descriptorSetLayoutInfo.pNext = &bindingFlagsInfo;

		if (vkCreateDescriptorSetLayout(
				eveDevice.device(),
				&descriptorSetLayoutInfo,
				nullptr,
				&descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	EveDescriptorSetLayout::~EveDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(eveDevice.device(), descriptorSetLayout, nullptr);
	}

	// *************** Descriptor Pool Builder *********************

	EveDescriptorPool::Builder &EveDescriptorPool::Builder::addPoolSize(
		VkDescriptorType descriptorType, uint32_t count)
	{
		poolSizes.push_back({descriptorType, count});
		return *this;
	}

	EveDescriptorPool::Builder &EveDescriptorPool::Builder::setPoolFlags(
		VkDescriptorPoolCreateFlags flags)
	{
		poolFlags = flags;
		return *this;
	}
	EveDescriptorPool::Builder &EveDescriptorPool::Builder::setMaxSets(uint32_t count)
	{
		maxSets = count;
		return *this;
	}

	std::unique_ptr<EveDescriptorPool> EveDescriptorPool::Builder::build() const
	{
		return std::make_unique<EveDescriptorPool>(eveDevice, maxSets, poolFlags, poolSizes);
	}

	// *************** Descriptor Pool *********************

	EveDescriptorPool::EveDescriptorPool(
		EveDevice &eveDevice,
		uint32_t maxSets,
		VkDescriptorPoolCreateFlags poolFlags,
		const std::vector<VkDescriptorPoolSize> &poolSizes)
		: eveDevice{eveDevice}
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		if (vkCreateDescriptorPool(eveDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	EveDescriptorPool::~EveDescriptorPool()
	{
		vkDestroyDescriptorPool(eveDevice.device(), descriptorPool, nullptr);
	}

	bool EveDescriptorPool::allocateDescriptor(
		const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const
	{
		uint32_t counts[1];
		counts[0] = 32; // Set 0 has a variable count descriptor with a maximum of 32 elements

		VkDescriptorSetVariableDescriptorCountAllocateInfo setCounts = {};
		setCounts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		setCounts.descriptorSetCount = 1;
		setCounts.pDescriptorCounts = counts;

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pNext = &setCounts;

		// Might want to create a "DescriptorPoolManager" class that handles this case, and builds
		// a new pool whenever an old pool fills up. But this is beyond our current scope
		if (vkAllocateDescriptorSets(eveDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	void EveDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const
	{
		vkFreeDescriptorSets(
			eveDevice.device(),
			descriptorPool,
			static_cast<uint32_t>(descriptors.size()),
			descriptors.data());
	}

	void EveDescriptorPool::resetPool()
	{
		vkResetDescriptorPool(eveDevice.device(), descriptorPool, 0);
	}

	// *************** Descriptor Writer *********************

	EveDescriptorWriter::EveDescriptorWriter(EveDescriptorSetLayout &setLayout, EveDescriptorPool &pool)
		: setLayout{setLayout}, pool{pool} {}

	EveDescriptorWriter &EveDescriptorWriter::writeBuffer(
		uint32_t binding, VkDescriptorBufferInfo *bufferInfo)
	{
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto &bindingDescription = setLayout.bindings[binding];

		assert(
			bindingDescription.descriptorCount == 1 &&
			"Binding single descriptor info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;

		writes.push_back(write);
		return *this;
	}

	EveDescriptorWriter &EveDescriptorWriter::writeImage(
		uint32_t binding, std::vector<VkDescriptorImageInfo> imageInfos)
	{
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto &bindingDescription = setLayout.bindings[binding];

		/*assert(
			bindingDescription.descriptorCount == 1 &&
			"Binding single descriptor info, but binding expects multiple");*/

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorCount = imageInfos.size();
		write.pImageInfo = imageInfos.data();

		writes.push_back(write);
		return *this;
	}

	bool EveDescriptorWriter::build(VkDescriptorSet &set)
	{
		bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
		if (!success)
		{
			return false;
		}
		overwrite(set);
		return true;
	}

	void EveDescriptorWriter::overwrite(VkDescriptorSet &set)
	{
		for (auto &write : writes)
		{
			write.dstSet = set;
		}
		vkUpdateDescriptorSets(pool.eveDevice.device(), writes.size(), writes.data(), 0, nullptr);
	}

} // namespace lve
