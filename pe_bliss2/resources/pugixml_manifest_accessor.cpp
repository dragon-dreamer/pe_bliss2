#include "pe_bliss2/resources/pugixml_manifest_accessor.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "pe_bliss2/pe_error.h"
#include "utilities/hash.h"
#include "utilities/scoped_guard.h"

#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include "external/pugixml/pugixml.hpp"

namespace pe_bliss::resources::pugixml
{

namespace
{
using ns_name_type = std::pair<std::string_view, std::string_view>;

struct ns_hasher
{
	std::size_t operator()(const ns_name_type& ns_name) const
	{
		std::size_t seed = std::hash<std::string_view>{}(ns_name.first);
		utilities::hash_combine(seed, std::hash<std::string_view>{}(ns_name.second));
		return seed;
	}
};

class namespace_registry_layer;

class namespace_registry
{
public:
	using ns_mapping_type = std::unordered_map<std::string_view,
		std::vector<std::string_view>>;

public:
	namespace_registry_layer push_namespaces();

private:
	void pop_namespaces()
	{
		for (std::string_view pushed : pushed_.back().namespaces)
		{
			auto it = ns_mapping_.find(pushed);
			it->second.pop_back();
			if (it->second.empty())
				ns_mapping_.erase(it);
		}
		if (pushed_.back().has_default_namespace)
			default_namespace_.pop_back();
		pushed_.pop_back();
	}
	
	std::string_view get_current_namespace(std::string_view alias) const
	{
		if (alias.empty())
			throw pe_error(manifest_loader_errc::invalid_xml);

		auto it = ns_mapping_.find(alias);
		if (it == ns_mapping_.cend())
			throw pe_error(manifest_loader_errc::invalid_xml);

		return it->second.back();
	}

	void push_namespace(std::string_view alias, std::string_view full_name)
	{
		if (alias.empty() || full_name.empty())
			throw pe_error(manifest_loader_errc::invalid_xml);

		if (!pushed_.back().namespaces.emplace(alias).second)
			throw pe_error(manifest_loader_errc::invalid_xml);

		ns_mapping_[alias].emplace_back(full_name);
	}

	void set_default(std::string_view default_ns)
	{
		if (default_ns.empty())
			throw pe_error(manifest_loader_errc::invalid_xml);

		if (pushed_.back().has_default_namespace)
			throw pe_error(manifest_loader_errc::invalid_xml);

		pushed_.back().has_default_namespace = true;
		default_namespace_.emplace_back(default_ns);
	}
	
	std::string_view get_default() const
	{
		return default_namespace_.back();
	}

private:
	struct layer
	{
		std::unordered_set<std::string_view> namespaces;
		bool has_default_namespace{};
	};

	friend class namespace_registry_layer;
	ns_mapping_type ns_mapping_;
	std::vector<layer> pushed_;
	std::vector<std::string_view> default_namespace_{ "" };
};

class namespace_registry_layer
{
public:
	namespace_registry_layer(const namespace_registry_layer&) = delete;
	namespace_registry_layer& operator=(const namespace_registry_layer&) = delete;
	namespace_registry_layer(namespace_registry_layer&&) = delete;
	namespace_registry_layer& operator=(namespace_registry_layer&&) = delete;

	explicit namespace_registry_layer(namespace_registry& registry) noexcept
		: registry_(registry)
	{
	}

	~namespace_registry_layer()
	{
		registry_.pop_namespaces();
	}

	void push_namespace(std::string_view alias, std::string_view full_name)
	{
		registry_.push_namespace(alias, full_name);
	}

	void set_default(std::string_view full_name)
	{
		registry_.set_default(full_name);
	}

	std::string_view get_default() const
	{
		return registry_.get_default();
	}

	std::string_view get_current_namespace(std::string_view alias) const
	{
		return registry_.get_current_namespace(alias);
	}

private:
	namespace_registry& registry_;
};

namespace_registry_layer namespace_registry::push_namespaces()
{
	pushed_.emplace_back();
	return namespace_registry_layer(*this);
}

class manifest_attr_impl final : public manifest_named_entity_interface
{
public:
	manifest_attr_impl(
		std::string_view ns,
		std::string_view name,
		const pugi::xml_attribute& attr)
		: ns_(ns)
		, name_(name)
		, attr_(attr)
	{
	}

	virtual std::string_view get_name() const override
	{
		return name_;
	}

	virtual std::string_view get_namespace() const override
	{
		return ns_;
	}

	virtual std::string_view get_value() const override
	{
		const auto* value = attr_.as_string(nullptr);
		if (!value)
			return {};

		return value;
	}

private:
	std::string_view name_;
	std::string_view ns_;
	pugi::xml_attribute attr_;
};

class xml_namespace_tracker;
using node_map_type = std::unordered_map<ns_name_type,
	std::vector<xml_namespace_tracker>,
	ns_hasher>;

class manifest_node_iterator_impl final : public manifest_node_iterator_interface
{
public:
	explicit manifest_node_iterator_impl(const node_map_type& nodes) noexcept
		: nodes_(nodes)
	{
	}

	virtual const manifest_node_interface* first_child(
		std::string_view ns_name, std::string_view name) override;

	virtual const manifest_node_interface* next_child() override;

private:
	const node_map_type& nodes_;
	std::vector<xml_namespace_tracker>::const_iterator it_{}, end_{};
};

class xml_namespace_tracker final : public manifest_node_interface
{
public:
	using attribute_map_type = std::unordered_map<ns_name_type,
		manifest_attr_impl, ns_hasher>;

public:
	xml_namespace_tracker() = default;

	xml_namespace_tracker(pugi::xml_node node, namespace_registry& ns_registry,
		std::size_t index, std::string_view ns, std::string_view name)
		: node_(std::move(node))
		, index_(index)
		, ns_(ns)
		, name_(name)
	{
		load_node(ns_registry);
	}

public:
	virtual std::string_view get_name() const override
	{
		return name_;
	}

	virtual std::string_view get_namespace() const override
	{
		return ns_;
	}

	virtual std::string_view get_value() const override
	{
		const auto* value = node_.text().as_string(nullptr);
		if (!value)
			return {};

		return value;
	}

	virtual std::size_t get_child_count() const override
	{
		return child_count_;
	}

	virtual manifest_node_iterator_interface_ptr
		get_iterator() const override
	{
		return std::make_unique<manifest_node_iterator_impl>(nodes_);
	}

	virtual const manifest_named_entity_interface* get_attribute(
		std::string_view ns_name, std::string_view name) const override
	{
		auto it = attributes_.find({ ns_name, name });
		if (it == attributes_.cend())
			return nullptr;

		return &it->second;
	}

	virtual std::size_t get_node_index() const noexcept override
	{
		return index_;
	}

private:
	void load_node(namespace_registry& ns_registry)
	{
		bool has_declaration = false;
		bool has_children = false;
		for (auto child : node_.children())
		{
			if (child.type() != pugi::node_element)
			{
				if (child.type() == pugi::node_pi
					|| child.type() == pugi::node_doctype)
				{
					throw pe_error(manifest_loader_errc::invalid_xml);
				}

				if (child.type() == pugi::node_declaration)
				{
					if (node_.type() != pugi::node_document
						|| has_declaration || has_children)
					{
						throw pe_error(manifest_loader_errc::invalid_xml);
					}
					has_declaration = true;
				}

				has_children = true;
				continue;
			}

			has_children = true;
			++child_count_;
			if (node_.type() == pugi::node_document)
			{
				if (child_count_ > 1u)
					throw pe_error(manifest_loader_errc::invalid_xml);
			}

			auto ns_layer = ns_registry.push_namespaces();
			for (auto attr : child.attributes())
			{
				std::string_view name = attr.name();
				if (name.starts_with(xmlns_prefix))
				{
					name.remove_prefix(xmlns_prefix.size());
					ns_layer.push_namespace(name, attr.value());
				}
				else if (name == default_ns_attr)
				{
					ns_layer.set_default(attr.value());
				}
			}

			auto node_ns_name = get_ns_name(child, ns_layer);
			auto& child_node = nodes_[node_ns_name].emplace_back(
				child, ns_registry,
				child_count_ - 1u, node_ns_name.first, node_ns_name.second);

			for (auto attr : child.attributes())
			{
				std::string_view name = attr.name();
				if (name.starts_with(xmlns_prefix) || name == default_ns_attr)
					continue;

				auto ns_name = get_ns_name(attr, ns_layer);
				if (!child_node.attributes_.emplace(std::piecewise_construct,
					std::forward_as_tuple(ns_name),
					std::forward_as_tuple(ns_name.first, ns_name.second, std::move(attr))).second)
				{
					throw pe_error(manifest_loader_errc::invalid_xml);
				}
			}
		}

		if (node_.type() == pugi::node_document)
		{
			if (!child_count_ || !has_declaration)
				throw pe_error(manifest_loader_errc::invalid_xml);
		}
	}

	template<typename Node>
	ns_name_type get_ns_name(const Node& node,
		const namespace_registry_layer& ns_layer)
	{
		std::string_view name = node.name();
		auto ns_pos = name.find(':');
		std::string_view ns_name;
		if (ns_pos != std::string_view::npos)
		{
			ns_name = ns_layer.get_current_namespace(
				name.substr(0u, ns_pos));
			name.remove_prefix(ns_pos + 1u);
			if (name.empty() || name.find(':') != std::string_view::npos)
				throw pe_error(manifest_loader_errc::invalid_xml);
		}
		else
		{
			ns_name = ns_layer.get_default();
		}

		return { ns_name, name };
	}

private:
	static constexpr std::string_view xmlns_prefix{ "xmlns:" };
	static constexpr std::string_view default_ns_attr{ "xmlns" };

private:
	node_map_type nodes_;
	attribute_map_type attributes_;
	pugi::xml_node node_;
	std::size_t index_{};
	std::string_view ns_;
	std::string_view name_;
	std::size_t child_count_{};
};

const manifest_node_interface* manifest_node_iterator_impl::first_child(
	std::string_view ns_name, std::string_view name)
{
	auto it = nodes_.find({ ns_name, name });
	if (it == nodes_.cend())
		return nullptr;

	it_ = it->second.cbegin();
	end_ = it->second.cend();
	return &*it_;
}

const manifest_node_interface* manifest_node_iterator_impl::next_child()
{
	if (++it_ == end_)
		return nullptr;

	return &*it_;
}

class manifest_node_impl;
} //namespace

namespace impl
{
struct pugixml_manifest_accessor_impl
{
public:
	xml_namespace_tracker ns_tracker;
	pugi::xml_document doc;
	std::vector<std::byte> xml_text;
	friend class manifest_node_impl;

public:
	bool parse(const buffers::input_buffer_ptr& buffer,
		error_list& errors)
	{
		try
		{
			xml_text.resize(buffer->physical_size());
			buffer->read(0, xml_text.size(), xml_text.data());
		}
		catch (const std::system_error&)
		{
			errors.add_error(manifest_loader_errc::invalid_xml);
			return false;
		}

		auto result = doc.load_buffer_inplace(xml_text.data(),
			xml_text.size(), pugi::parse_full);
		if (!result)
		{
			errors.add_error(manifest_loader_errc::invalid_xml);
			return false;
		}

		namespace_registry registry;
		try
		{
			ns_tracker = xml_namespace_tracker(doc, registry, 0u, {}, {});
		}
		catch (const pe_error& e)
		{
			errors.add_error(e.code());
			return false;
		}

		return true;
	}
};
} //namespace impl

pugixml_manifest_accessor::pugixml_manifest_accessor(const buffers::input_buffer_ptr& buffer)
	: impl_(std::make_unique<impl::pugixml_manifest_accessor_impl>())
{
	if (!impl_->parse(buffer, errors_))
		impl_.reset();
}

const manifest_node_interface* pugixml_manifest_accessor::get_root() const
{
	if (!impl_)
		return nullptr;

	return &impl_->ns_tracker;
}

manifest_accessor_interface_ptr parse_manifest(
	const buffers::input_buffer_ptr& buffer)
{
	return std::make_shared<pugixml_manifest_accessor>(buffer);
}

} //namespace pe_bliss::resources::pugixml
