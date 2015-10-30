#include <boost/python.hpp>
#include <boost/python/detail/wrap_python.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/args.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/format.hpp>

#include <typeinfo>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <wchar.h>

#include "Vmd.h"
#include "Pmx.h"

// wrap-code from: http://stackoverflow.com/questions/18882089/wrapping-arrays-in-boost-python
namespace boost {
	namespace python {
		namespace detail {

			template <typename> struct array_trait;

			/// @brief Type that proxies to an array.
			template <typename T>
			class array_proxy
			{
			public:
				// Types
				typedef T           value_type;
				typedef T*          iterator;
				typedef T&          reference;
				typedef std::size_t size_type;

				/// @brief Empty constructor.
				array_proxy()
					: ptr_(0),
					length_(0)
				{}

				/// @brief Construct with iterators.
				template <typename Iterator>
				array_proxy(Iterator begin, Iterator end)
					: ptr_(&*begin),
					length_(std::distance(begin, end))
				{}

				/// @brief Construct with with start and size.
				array_proxy(reference begin, std::size_t length)
					: ptr_(&begin),
					length_(length)
				{}

				// Iterator support.
				iterator begin()               { return ptr_; }
				iterator end()                 { return ptr_ + length_; }

				// Element access.
				reference operator[](size_t i) { return ptr_[i]; }

				// Capacity.
				size_type size()               { return length_; }

			private:
				T* ptr_;
				std::size_t length_;
			};

			/// @brief Make an array_proxy.
			template <typename T>
			array_proxy<typename array_trait<T>::element_type>
				make_array_proxy(T& array)
			{
				return array_proxy<typename array_trait<T>::element_type>(
					array[0],
					array_trait<T>::static_size);
			}

			/// @brief Policy type for referenced indexing, meeting the DerivedPolicies
			///        requirement of boost::python::index_suite.
			/// 
			/// @note Requires Container to support:
			///          - value_type and size_type types,
			///          - value_type is default constructable and copyable,
			///          - element access via operator[],
			///          - Default constructable, iterator constructable,
			///          - begin(), end(), and size() member functions
			template <typename Container>
			class ref_index_suite
				: public indexing_suite < Container, ref_index_suite<Container> >
			{
			public:

				typedef typename Container::value_type data_type;
				typedef typename Container::size_type  index_type;
				typedef typename Container::size_type  size_type;

				// Element access and manipulation.

				/// @brief Get element from container.
				static data_type&
					get_item(Container& container, index_type index)
				{
					return container[index];
				}

				/// @brief Set element from container.
				static void
					set_item(Container& container, index_type index, const data_type& value)
				{
					container[index] = value;
				}

				/// @brief Reset index to default value.
				static void
					delete_item(Container& container, index_type index)
				{
					set_item(container, index, data_type());
				};

				// Slice support.

				/// @brief Get slice from container.
				///
				/// @return Python object containing
				static object
					get_slice(Container& container, index_type from, index_type to)
				{
					if (from > to) return list();

					// Return copy, as container only references its elements.
					list list;
					while (from != to) list.append(container[from++]);
					return list;
				};

				/// @brief Set a slice in container with a given value.
				static void
					set_slice(
					Container& container, index_type from,
					index_type to, const data_type& value
					)
				{
					// If range is invalid, return early.
					if (from > to) return;

					// Populate range with value.
					while (from < to) container[from++] = value;
				}

				/// @brief Set a slice in container with another range.
				template <class Iterator>
				static void
					set_slice(
					Container& container, index_type from,
					index_type to, Iterator first, Iterator last
					)
				{
					// If range is invalid, return early.
					if (from > to) return;

					// Populate range with other range.
					while (from < to) container[from++] = *first++;
				}

				/// @brief Reset slice to default values.
				static void
					delete_slice(Container& container, index_type from, index_type to)
				{
					set_slice(container, from, to, data_type());
				}

				// Capacity.

				/// @brief Get size of container.
				static std::size_t size(Container& container) { return container.size(); }

				/// @brief Check if a value is within the container.
				template <class T>
				static bool
					contains(Container& container, const T& value)
				{
					return std::find(container.begin(), container.end(), value)
						!= container.end();
				}

				/// @brief Minimum index supported for container.
				static index_type
					get_min_index(Container& /*container*/)
				{
					return 0;
				}

				/// @brief Maximum index supported for container.
				static index_type get_max_index(Container& container)
				{
					return size(container);
				}

				// Misc.

				/// @brief Convert python index (could be negative) to a valid container
				///        index with proper boundary checks.
				static index_type convert_index(Container& container, PyObject* object)
				{
					extract<index_type> py_index(object);

					// If py_index cannot extract a long, then type the type is wrong so
					// set error and return early.
					if (!py_index.check())
					{
						PyErr_SetString(PyExc_TypeError, "Invalid index type");
						throw_error_already_set();
						return index_type();
					}

					// Extract index.
					index_type index = static_cast<index_type>(py_index());

					// Adjust negative index.
					if (index < 0)
						index += static_cast<index_type>(container.size());

					// Boundary check.
					if (index >= static_cast<index_type>(container.size()) || index < 0)
					{
						PyErr_SetString(PyExc_IndexError, "Index out of range");
						throw_error_already_set();
					}

					return index;
				}
			};

			/// @brief Trait for arrays.
			template <typename T>
			struct array_trait_impl;

			// Specialize for native array.
			template <typename T, std::size_t N>
			struct array_trait_impl < T[N] >
			{
				typedef T element_type;
				enum { static_size = N };
				typedef array_proxy<element_type> proxy_type;
				typedef default_call_policies policy;
				typedef boost::mpl::vector<array_proxy<element_type> > signature;
			};

			// Specialize boost::array to use the native array trait.
			template <typename T, std::size_t N>
			struct array_trait_impl<boost::array<T, N> >
				: public array_trait_impl < T[N] >
			{};

			// @brief Specialize for member objects to use and modify non member traits.
			template <typename T, typename C>
			struct array_trait_impl<T(C::*)>
				: public array_trait_impl < T >
			{
				typedef with_custodian_and_ward_postcall <
				0, // return object (custodian)
				1  // self or this (ward)
				> policy;

				// Append the class to the signature.
				typedef typename boost::mpl::push_back <
					typename array_trait_impl<T>::signature, C& > ::type signature;
			};

			/// @brief Trait class used to deduce array information, policies, and 
			///        signatures
			template <typename T>
			struct array_trait :
				public array_trait_impl < typename boost::remove_pointer<T>::type >
			{
				typedef T native_type;
			};

			/// @brief Functor used used convert an array to an array_proxy for
			///        non-member objects.
			template <typename Trait>
			struct array_proxy_getter
			{
			public:
				typedef typename Trait::native_type native_type;
				typedef typename Trait::proxy_type proxy_type;

				/// @brief Constructor.
				array_proxy_getter(native_type array) : array_(array) {}

				/// @brief Return an array_proxy for a member array object.
				template <typename C>
				proxy_type operator()(C& c) { return make_array_proxy(c.*array_); }

				/// @brief Return an array_proxy for a non-member array object.
				proxy_type operator()() { return make_array_proxy(*array_); }
			private:
				native_type array_;
			};

			/// @brief Conditionally register a type with Boost.Python.
			template <typename Trait>
			void register_array_proxy()
			{
				typedef typename Trait::element_type element_type;
				typedef typename Trait::proxy_type proxy_type;

				// If type is already registered, then return early.
				namespace python = boost::python;
				bool is_registered = (0 != python::converter::registry::query(
					python::type_id<proxy_type>())->to_python_target_type());
				if (is_registered) return;

				//// Otherwise, register the type as an internal type.
				std::string type_name = std::string("_") + typeid(element_type).name();
				class_<proxy_type>(type_name.c_str(), no_init)
					.def(ref_index_suite<proxy_type>());
			}

			/// @brief Create a callable Boost.Python object that will return an
			///        array_proxy type when called.
			///
			/// @note This function will conditionally register array_proxy types
			///       for conversion within Boost.Python.  The array_proxy will
			///       extend the life of the object from which it was called.
			///       For example, if `foo` is an object, and `vars` is an array,
			///       then the object returned from `foo.vars` will extend the life
			///       of `foo`.
			template <typename Array>
			object make_array_aux(Array array)
			{
				typedef array_trait<Array> trait_type;
				// Register an array proxy.
				register_array_proxy<trait_type>();

				// Create function.
				return make_function(
					array_proxy_getter<trait_type>(array),
					typename trait_type::policy(),
					typename trait_type::signature());
			}

		} // namespace detail
	}
}


/// @brief Create a callable Boost.Python object from an array.
template <typename T>
boost::python::object make_array(T array)
{
	return boost::python::detail::make_array_aux(array);
}


namespace
{

	using namespace boost::python;
	using namespace vmd;
	using namespace pmx;

	template <typename T>
	void make_vector() {
		class_<std::vector<T>>((std::string("vec_") + typeid(T).name()).c_str())
			.def(vector_indexing_suite<std::vector<T> >())
			;
	}

	boost::python::list get_bone_frame_position(VmdBoneFrame& frame)
	{
		boost::python::list py_list;
		py_list.append(frame.position[0]);
		py_list.append(frame.position[1]);
		py_list.append(frame.position[2]);
		return py_list;
	}

	void set_bone_frame_position(VmdBoneFrame& frame, object p)
	{
		const boost::python::tuple tp = extract<tuple>(p)();
		if (len(tp) < 3) {
			PyErr_SetString(PyExc_IndexError, "index out of range");
			throw boost::python::error_already_set();
		}
		frame.position[0] = static_cast<float>(extract<double>(tp[0]));
		frame.position[1] = static_cast<float>(extract<double>(tp[1]));
		frame.position[2] = static_cast<float>(extract<double>(tp[2]));
	}

	boost::python::list get_bone_frame_orientation(VmdBoneFrame& frame)
	{
		boost::python::list py_list;
		py_list.append(frame.orientation[0]);
		py_list.append(frame.orientation[1]);
		py_list.append(frame.orientation[2]);
		py_list.append(frame.orientation[3]);
		return py_list;
	}

	void set_morph_name(PmxMorph& morph, const std::wstring& p) {
		morph.morph_name = p;
	}

	boost::python::object get_morph_name(PmxMorph& morph) {
		boost::python::str str(morph.morph_name);
		return str;
	}

	void set_bone_frame_orientation(VmdBoneFrame& frame, object p)
	{
		const boost::python::tuple tp = extract<tuple>(p)();
		if (len(tp) < 3) {
			PyErr_SetString(PyExc_IndexError, "index out of range");
			throw boost::python::error_already_set();
		}
		frame.orientation[0] = static_cast<float>(extract<double>(tp[0]));
		frame.orientation[1] = static_cast<float>(extract<double>(tp[1]));
		frame.orientation[2] = static_cast<float>(extract<double>(tp[2]));
		frame.orientation[3] = static_cast<float>(extract<double>(tp[3]));
	}

	void as_center_bone(PmxBone& bone)
	{
		bone.bone_name = std::wstring(L"センター");
		bone.parent_index = -1;
		bone.bone_flag = (0x0001 | 0x0002 | 0x0004 | 0x0008 | 0x0010);
		bone.offset[1] = 1;
	}

	bool save_vmd_to_file(VmdMotion& motion, const char* filepath)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
		std::u16string utf16str = convert.from_bytes(filepath);
		return motion.SaveToFile(utf16str);
	}

	bool save_pmx_to_file(PmxModel& pmx, const char* filepath)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
		std::u16string utf16str = convert.from_bytes(filepath);
		try {
			std::ofstream stream(utf16str.c_str(), std::ios::binary);
			pmx.Write(&stream);
			stream.close();
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	void add_bone_frame(VmdMotion& motion, VmdBoneFrame& bone_frame)
	{
		for (int n = 0; n < 4; ++n) {
			for (int m = 0; m < 4; ++m) {
				bone_frame.interpolation[n][0][m] = 20;
				bone_frame.interpolation[n][1][m] = 20;
			}
			for (int m = 0; m < 4; ++m) {
				bone_frame.interpolation[n][2][m] = 107;
				bone_frame.interpolation[n][3][m] = 107;
			}
		}
		motion.bone_frames.push_back(bone_frame);
	}

	std::shared_ptr<PmxVertexSkinning> to_skinning_bdef1(const PmxVertexSkinningBDEF1& src) {
		// TODO copy
		return std::make_shared<PmxVertexSkinningBDEF1>();
	}

	std::shared_ptr<PmxVertexSkinning> to_skinning_bdef2(const PmxVertexSkinningBDEF2& src) {
		// TODO copy
		return std::make_shared<PmxVertexSkinningBDEF2>();
	}

	std::shared_ptr<PmxVertexSkinning> to_skinning_bdef4(const PmxVertexSkinningBDEF4& src) {
		// TODO copy
		return std::make_shared<PmxVertexSkinningBDEF4>();
	}

	std::shared_ptr<PmxVertexSkinning> to_skinning_sdef(const PmxVertexSkinningSDEF& src) {
		// TODO copy
		return std::make_shared<PmxVertexSkinningSDEF>();
	}

	std::shared_ptr<PmxVertexSkinning> to_skinning_qdef(const PmxVertexSkinningQDEF& src) {
		// TODO copy
		return std::make_shared<PmxVertexSkinningQDEF>();
	}

BOOST_PYTHON_MODULE(mmformat)
{
		class_<VmdBoneFrame>("VmdBoneFrame")
			.add_property("name", make_getter(&VmdBoneFrame::name), make_setter(&VmdBoneFrame::name))
			.add_property("frame", make_getter(&VmdBoneFrame::frame), make_setter(&VmdBoneFrame::frame))
			.add_property("position", &get_bone_frame_position, &set_bone_frame_position)
			.add_property("orientation", &get_bone_frame_orientation, &set_bone_frame_orientation)
			//.add_property("interpolation", make_getter(&VmdBoneFrame::interpolation), make_setter(&VmdBoneFrame::interpolation))
			;

		class_<VmdFaceFrame>("VmdFaceFrame")
			.add_property("face_name", make_getter(&VmdFaceFrame::face_name), make_setter(&VmdFaceFrame::face_name))
			.add_property("weight", make_getter(&VmdFaceFrame::weight), make_setter(&VmdFaceFrame::weight))
			.add_property("frame", make_getter(&VmdFaceFrame::frame), make_setter(&VmdFaceFrame::frame))
			;

		class_<VmdCameraFrame>("VmdCameraFrame")
			.add_property("frame", make_getter(&VmdCameraFrame::frame), make_setter(&VmdCameraFrame::frame))
			.add_property("distance", make_getter(&VmdCameraFrame::distance), make_setter(&VmdCameraFrame::distance))
			.add_property("position", make_array(&VmdCameraFrame::position))
			.add_property("orientation", make_array(&VmdCameraFrame::orientation))
			//.add_property("interpolation", make_array(&VmdCameraFrame::interpolation))
			.add_property("angle", make_getter(&VmdCameraFrame::angle), make_setter(&VmdCameraFrame::angle))
			.add_property("unknown", make_array(&VmdCameraFrame::unknown))
			;
		
		class_<VmdLightFrame>("VmdLightFrame")
			.add_property("frame", make_getter(&VmdLightFrame::frame), make_setter(&VmdLightFrame::frame))
			.add_property("color", make_array(&VmdLightFrame::color))
			.add_property("position", make_array(&VmdLightFrame::position))
			;

		class_<VmdIkFrame>("VmdIkFrame")
			.add_property("frame", make_getter(&VmdIkFrame::frame), make_setter(&VmdIkFrame::frame))
			.add_property("display", make_getter(&VmdIkFrame::display), make_setter(&VmdIkFrame::display))
			;

		make_vector<VmdBoneFrame>();
		make_vector<VmdCameraFrame>();
		make_vector<VmdFaceFrame>();
		make_vector<VmdIkFrame>();

		class_<VmdMotion>("VmdMotion")
			.add_property("model_name", make_getter(&VmdMotion::model_name), make_setter(&VmdMotion::model_name))
			.add_property("version", make_getter(&VmdMotion::version), make_setter(&VmdMotion::version))
			.add_property("bone_frames", make_getter(&VmdMotion::bone_frames))
			.add_property("face_frames", make_getter(&VmdMotion::face_frames))
			.add_property("camera_frames", make_getter(&VmdMotion::camera_frames))
			.add_property("light_frames", make_getter(&VmdMotion::light_frames))
			.add_property("ik_frames", make_getter(&VmdMotion::ik_frames))
			//.def("add_bone_frame", &add_bone_frame)
			.def("save_to_file", &save_vmd_to_file)
			;
		//-----------------------------------------------------------------


		class_<PmxSetting>("PmxSetting")
			.add_property("encoding", make_getter(&PmxSetting::encoding), make_setter(&PmxSetting::encoding))
			.add_property("uv", make_getter(&PmxSetting::uv), make_setter(&PmxSetting::uv))
			.add_property("vertex_index_size", make_getter(&PmxSetting::vertex_index_size), make_setter(&PmxSetting::vertex_index_size))
			.add_property("texture_index_size", make_getter(&PmxSetting::texture_index_size), make_setter(&PmxSetting::texture_index_size))
			.add_property("material_index_size", make_getter(&PmxSetting::material_index_size), make_setter(&PmxSetting::material_index_size))
			.add_property("bone_index_size", make_getter(&PmxSetting::bone_index_size), make_setter(&PmxSetting::bone_index_size))
			.add_property("morph_index_size", make_getter(&PmxSetting::morph_index_size), make_setter(&PmxSetting::morph_index_size))
			.add_property("rigidbody_index_size", make_getter(&PmxSetting::rigidbody_index_size), make_setter(&PmxSetting::rigidbody_index_size))
			;

		enum_<PmxVertexSkinningType>("PmxVertexSkinningType")
			.value("BDEF1", PmxVertexSkinningType::BDEF1)
			.value("BDEF2", PmxVertexSkinningType::BDEF2)
			.value("BDEF4", PmxVertexSkinningType::BDEF4)
			.value("SDEF", PmxVertexSkinningType::SDEF)
			.value("QDEF", PmxVertexSkinningType::QDEF)
			.export_values()
			;

		class_<PmxVertexSkinning, boost::noncopyable>("PmxVertexSkinning", no_init);

		register_ptr_to_python<std::shared_ptr<PmxVertexSkinning>>();

		class_<PmxVertexSkinningBDEF1, bases<PmxVertexSkinning> >("PmxVertexSkinningBDEF1")
			.add_property("bone_index", make_getter(&PmxVertexSkinningBDEF1::bone_index), make_setter(&PmxVertexSkinningBDEF1::bone_index))
			.def("to_skinning", to_skinning_bdef1, (arg("self")))
			;

		class_<PmxVertexSkinningBDEF2, bases<PmxVertexSkinning> >("PmxVertexSkinningBDEF2")
			.add_property("bone_index1", make_getter(&PmxVertexSkinningBDEF2::bone_index1), make_setter(&PmxVertexSkinningBDEF2::bone_index1))
			.add_property("bone_index2", make_getter(&PmxVertexSkinningBDEF2::bone_index2), make_setter(&PmxVertexSkinningBDEF2::bone_index2))
			.add_property("bone_weight", make_getter(&PmxVertexSkinningBDEF2::bone_weight), make_setter(&PmxVertexSkinningBDEF2::bone_weight))
			.def("to_skinning", to_skinning_bdef2, (arg("self")))
			;

		class_<PmxVertexSkinningBDEF4, bases<PmxVertexSkinning> >("PmxVertexSkinningBDEF4")
			.add_property("bone_index1", make_getter(&PmxVertexSkinningBDEF4::bone_index1), make_setter(&PmxVertexSkinningBDEF4::bone_index1))
			.add_property("bone_index2", make_getter(&PmxVertexSkinningBDEF4::bone_index2), make_setter(&PmxVertexSkinningBDEF4::bone_index2))
			.add_property("bone_index3", make_getter(&PmxVertexSkinningBDEF4::bone_index3), make_setter(&PmxVertexSkinningBDEF4::bone_index3))
			.add_property("bone_index4", make_getter(&PmxVertexSkinningBDEF4::bone_index4), make_setter(&PmxVertexSkinningBDEF4::bone_index4))
			.add_property("bone_weight1", make_getter(&PmxVertexSkinningBDEF4::bone_weight1), make_setter(&PmxVertexSkinningBDEF4::bone_weight1))
			.add_property("bone_weight2", make_getter(&PmxVertexSkinningBDEF4::bone_weight2), make_setter(&PmxVertexSkinningBDEF4::bone_weight2))
			.add_property("bone_weight3", make_getter(&PmxVertexSkinningBDEF4::bone_weight3), make_setter(&PmxVertexSkinningBDEF4::bone_weight3))
			.add_property("bone_weight4", make_getter(&PmxVertexSkinningBDEF4::bone_weight4), make_setter(&PmxVertexSkinningBDEF4::bone_weight4))
			.def("to_skinning", to_skinning_bdef4, (arg("self")))
			;

		class_<PmxVertexSkinningSDEF, bases<PmxVertexSkinning> >("PmxVertexSkinningSDEF")
			.add_property("bone_index1", make_getter(&PmxVertexSkinningSDEF::bone_index1), make_setter(&PmxVertexSkinningSDEF::bone_index1))
			.add_property("bone_index2", make_getter(&PmxVertexSkinningSDEF::bone_index2), make_setter(&PmxVertexSkinningSDEF::bone_index2))
			.add_property("bone_weight", make_getter(&PmxVertexSkinningSDEF::bone_weight), make_setter(&PmxVertexSkinningSDEF::bone_weight))
			.add_property("sdef_c", make_array(&PmxVertexSkinningSDEF::sdef_c))
			.add_property("sdef_r0", make_array(&PmxVertexSkinningSDEF::sdef_r0))
			.add_property("sdef_r1", make_array(&PmxVertexSkinningSDEF::sdef_r1))
			.def("to_skinning", to_skinning_sdef, (arg("self")) )
			;

		class_<PmxVertexSkinningQDEF, bases<PmxVertexSkinning> >("PmxVertexSkinningQDEF")
			.add_property("bone_index1", make_getter(&PmxVertexSkinningQDEF::bone_index1), make_setter(&PmxVertexSkinningQDEF::bone_index1))
			.add_property("bone_index2", make_getter(&PmxVertexSkinningQDEF::bone_index2), make_setter(&PmxVertexSkinningQDEF::bone_index2))
			.add_property("bone_index3", make_getter(&PmxVertexSkinningQDEF::bone_index3), make_setter(&PmxVertexSkinningQDEF::bone_index3))
			.add_property("bone_index4", make_getter(&PmxVertexSkinningQDEF::bone_index4), make_setter(&PmxVertexSkinningQDEF::bone_index4))
			.add_property("bone_weight1", make_getter(&PmxVertexSkinningQDEF::bone_weight1), make_setter(&PmxVertexSkinningQDEF::bone_weight1))
			.add_property("bone_weight2", make_getter(&PmxVertexSkinningQDEF::bone_weight2), make_setter(&PmxVertexSkinningQDEF::bone_weight2))
			.add_property("bone_weight3", make_getter(&PmxVertexSkinningQDEF::bone_weight3), make_setter(&PmxVertexSkinningQDEF::bone_weight3))
			.add_property("bone_weight4", make_getter(&PmxVertexSkinningQDEF::bone_weight4), make_setter(&PmxVertexSkinningQDEF::bone_weight4))
			.def("to_skinning", to_skinning_qdef, (arg("self")) )
			;

		class_<PmxVertex>("PmxVertex")
			.add_property("position", make_array(&PmxVertex::position))
			.add_property("normal", make_array(&PmxVertex::normal))
			.add_property("uv", make_array(&PmxVertex::uv))
			//.add_property("uva", make_array(&PmxVertex::uva))
			.add_property("skinning_type", make_getter(&PmxVertex::skinning_type), make_setter(&PmxVertex::skinning_type))
			.add_property("skinning", make_getter(&PmxVertex::skinning), make_setter(&PmxVertex::skinning))
			.add_property("edge", make_getter(&PmxVertex::edge), make_setter(&PmxVertex::edge))
			;

		class_<PmxMaterial>("PmxMaterial")
			.add_property("material_name", make_getter(&PmxMaterial::material_name), make_setter(&PmxMaterial::material_name))
			.add_property("material_english_name", make_getter(&PmxMaterial::material_english_name), make_setter(&PmxMaterial::material_english_name))
			.add_property("diffuse", make_array(&PmxMaterial::diffuse))
			.add_property("specular", make_array(&PmxMaterial::specular))
			.add_property("specularity", make_getter(&PmxMaterial::specularlity), make_setter(&PmxMaterial::specularlity))
			.add_property("ambient", make_array(&PmxMaterial::ambient))
			.add_property("edge_color", make_array(&PmxMaterial::edge_color))
			.add_property("edge_size", make_getter(&PmxMaterial::edge_size), make_setter(&PmxMaterial::edge_size))
			.add_property("diffuse_texture_index", make_getter(&PmxMaterial::diffuse_texture_index), make_setter(&PmxMaterial::diffuse_texture_index))
			.add_property("sphere_texture_index", make_getter(&PmxMaterial::sphere_texture_index), make_setter(&PmxMaterial::sphere_texture_index))
			.add_property("sphere_op_mode", make_getter(&PmxMaterial::sphere_op_mode), make_setter(&PmxMaterial::sphere_op_mode))
			.add_property("common_toon_flag", make_getter(&PmxMaterial::common_toon_flag), make_setter(&PmxMaterial::common_toon_flag))
			.add_property("toon_texture_index", make_getter(&PmxMaterial::toon_texture_index), make_setter(&PmxMaterial::toon_texture_index))
			.add_property("memo", make_getter(&PmxMaterial::memo), make_setter(&PmxMaterial::memo))
			.add_property("index_count", make_getter(&PmxMaterial::index_count), make_setter(&PmxMaterial::index_count))
			;

		class_<PmxIkLink>("PmxIkLink")
			.add_property("link_target", make_getter(&PmxIkLink::link_target), make_setter(&PmxIkLink::link_target))
			.add_property("angle_lock", make_getter(&PmxIkLink::angle_lock), make_setter(&PmxIkLink::angle_lock))
			.add_property("max_radian", make_array(&PmxIkLink::max_radian))
			.add_property("min_radian", make_array(&PmxIkLink::min_radian))
			;

		make_vector<PmxIkLink>();

		class_<PmxBone>("PmxBone")
			.add_property("bone_name", make_getter(&PmxBone::bone_name), make_setter(&PmxBone::bone_name))
			.add_property("bone_english_name", make_getter(&PmxBone::bone_english_name), make_setter(&PmxBone::bone_english_name))
			.add_property("position", make_array(&PmxBone::position))
			.add_property("parent_index", make_getter(&PmxBone::parent_index), make_setter(&PmxBone::parent_index))
			.add_property("level", make_getter(&PmxBone::level), make_setter(&PmxBone::level))
			.add_property("bone_flag", make_getter(&PmxBone::bone_flag), make_setter(&PmxBone::bone_flag))
			.add_property("offset", make_array(&PmxBone::offset))
			.add_property("target_index", make_getter(&PmxBone::target_index), make_setter(&PmxBone::target_index))
			.add_property("grant_parent_index", make_getter(&PmxBone::grant_parent_index), make_setter(&PmxBone::grant_parent_index))
			.add_property("grant_weight", make_getter(&PmxBone::grant_weight), make_setter(&PmxBone::grant_weight))
			.add_property("lock_axis_orientation", make_array(&PmxBone::lock_axis_orientation))
			.add_property("local_axis_x_orientation", make_array(&PmxBone::local_axis_x_orientation))
			.add_property("local_axis_y_orientation", make_array(&PmxBone::local_axis_y_orientation))
			.add_property("key", make_getter(&PmxBone::key), make_setter(&PmxBone::key))
			.add_property("ik_target_bone_index", make_getter(&PmxBone::ik_target_bone_index), make_setter(&PmxBone::ik_target_bone_index))
			.add_property("ik_loop", make_getter(&PmxBone::ik_loop), make_setter(&PmxBone::ik_loop))
			.add_property("ik_loop_angle_limit", make_getter(&PmxBone::ik_loop_angle_limit), make_setter(&PmxBone::ik_loop_angle_limit))
			.add_property("ik_link_count", make_getter(&PmxBone::ik_link_count), make_setter(&PmxBone::ik_link_count))
			.add_property("ik_links", make_getter(&PmxBone::ik_links), make_setter(&PmxBone::ik_links))
			.def("as_center_bone", as_center_bone)
			;

		enum_<MorphType>("MorphType")
			.value("Group", MorphType::Group)
			.value("Vertex", MorphType::Vertex)
			.value("Bone", MorphType::Bone)
			.value("UV", MorphType::UV)
			.value("AdditionalUV1", MorphType::AdditionalUV1)
			.value("AdditionalUV2", MorphType::AdditionalUV2)
			.value("AdditionalUV3", MorphType::AdditionalUV3)
			.value("AdditionalUV4", MorphType::AdditionalUV4)
			.value("Matrial", MorphType::Matrial)
			.value("Flip", MorphType::Flip)
			.value("Implus", MorphType::Implus)
			.export_values()
			;

		enum_<MorphCategory>("MorphCategory")
			.value("ReservedCategory", MorphCategory::ReservedCategory)
			.value("Eyebrow", MorphCategory::Eyebrow)
			.value("Eye", MorphCategory::Eye)
			.value("Mouth", MorphCategory::Mouth)
			.value("Other", MorphCategory::Other)
			.export_values()
			;

		class_<PmxMorphOffset, boost::noncopyable>("PmxMorphOffset", no_init);

		class_<PmxMorphVertexOffset, bases<PmxMorphOffset> >("PmxMorphVertexOffset")
			.add_property("vertex_index", make_getter(&PmxMorphVertexOffset::vertex_index), make_setter(&PmxMorphVertexOffset::vertex_index))
			.add_property("position_offset", make_array(&PmxMorphVertexOffset::position_offset))
			;

		class_<PmxMorphUVOffset, bases<PmxMorphOffset> >("PmxMorphUVOffset")
			.add_property("vertex_index", make_getter(&PmxMorphUVOffset::vertex_index), make_setter(&PmxMorphUVOffset::vertex_index))
			.add_property("uv_offset", make_array(&PmxMorphUVOffset::uv_offset))
			;

		class_<PmxMorphBoneOffset, bases<PmxMorphOffset> >("PmxMorphBoneOffset")
			.add_property("bone_index", make_getter(&PmxMorphBoneOffset::bone_index), make_setter(&PmxMorphBoneOffset::bone_index))
			.add_property("translation", make_array(&PmxMorphBoneOffset::translation))
			.add_property("rotation", make_array(&PmxMorphBoneOffset::rotation))
			;

		class_<PmxMorphMaterialOffset, bases<PmxMorphOffset> >("PmxMorphMaterialOffset")
			.add_property("material_index", make_getter(&PmxMorphMaterialOffset::material_index), make_setter(&PmxMorphMaterialOffset::material_index))
			.add_property("offset_operation", make_getter(&PmxMorphMaterialOffset::offset_operation), make_setter(&PmxMorphMaterialOffset::offset_operation))
			.add_property("diffuse", make_array(&PmxMorphMaterialOffset::diffuse))
			.add_property("specular", make_array(&PmxMorphMaterialOffset::specular))
			.add_property("specularity", make_getter(&PmxMorphMaterialOffset::specularity), make_setter(&PmxMorphMaterialOffset::specularity))
			.add_property("ambient", make_array(&PmxMorphMaterialOffset::ambient))
			.add_property("edge_color", make_array(&PmxMorphMaterialOffset::edge_color))
			.add_property("edge_size", make_getter(&PmxMorphMaterialOffset::edge_size), make_setter(&PmxMorphMaterialOffset::edge_size))
			.add_property("texture_argb", make_array(&PmxMorphMaterialOffset::texture_argb))
			.add_property("sphere_texture_argb", make_array(&PmxMorphMaterialOffset::sphere_texture_argb))
			.add_property("toon_texture_argb", make_array(&PmxMorphMaterialOffset::toon_texture_argb))
			;

		class_<PmxMorphGroupOffset, bases<PmxMorphOffset> >("PmxMorphGroupOffset")
			.add_property("morph_index", make_getter(&PmxMorphGroupOffset::morph_index), make_setter(&PmxMorphGroupOffset::morph_index))
			.add_property("morph_weight", make_getter(&PmxMorphGroupOffset::morph_weight), make_setter(&PmxMorphGroupOffset::morph_weight))
			;

		class_<PmxMorphFlipOffset, bases<PmxMorphOffset> >("PmxMorphFlipOffset")
			.add_property("morph_index", make_getter(&PmxMorphFlipOffset::morph_index), make_setter(&PmxMorphFlipOffset::morph_index))
			.add_property("morph_value", make_getter(&PmxMorphFlipOffset::morph_value), make_setter(&PmxMorphFlipOffset::morph_value))
			;

		class_<PmxMorphImplusOffset, bases<PmxMorphOffset> >("PmxMorphImplusOffset")
			.add_property("rigid_body_index", make_getter(&PmxMorphImplusOffset::rigid_body_index), make_setter(&PmxMorphImplusOffset::rigid_body_index))
			.add_property("is_local", make_getter(&PmxMorphImplusOffset::is_local), make_setter(&PmxMorphImplusOffset::is_local))
			.add_property("velocity", make_array(&PmxMorphImplusOffset::velocity))
			.add_property("angular_torque", make_array(&PmxMorphImplusOffset::angular_torque))
			;

		make_vector<PmxMorphVertexOffset>();
		make_vector<PmxMorphUVOffset>();
		make_vector<PmxMorphBoneOffset>();
		make_vector<PmxMorphMaterialOffset>();
		make_vector<PmxMorphGroupOffset>();
		make_vector<PmxMorphFlipOffset>();
		make_vector<PmxMorphImplusOffset>();

		class_<PmxMorph>("PmxMorph")
			.add_property("morph_name", get_morph_name, set_morph_name)
			.add_property("morph_english_name", make_getter(&PmxMorph::morph_english_name), make_setter(&PmxMorph::morph_english_name))
			.add_property("category", make_getter(&PmxMorph::category), make_setter(&PmxMorph::category))
			.add_property("morph_type", make_getter(&PmxMorph::morph_type), make_setter(&PmxMorph::morph_type))
			.add_property("offset_count", make_getter(&PmxMorph::offset_count), make_setter(&PmxMorph::offset_count))
			.add_property("vertex_offsets", make_getter(&PmxMorph::vertex_offsets), make_setter(&PmxMorph::vertex_offsets))
			.add_property("uv_offsets", make_getter(&PmxMorph::uv_offsets), make_setter(&PmxMorph::uv_offsets))
			.add_property("bone_offsets", make_getter(&PmxMorph::bone_offsets), make_setter(&PmxMorph::bone_offsets))
			.add_property("material_offsets", make_getter(&PmxMorph::material_offsets), make_setter(&PmxMorph::material_offsets))
			.add_property("group_offsets", make_getter(&PmxMorph::group_offsets), make_setter(&PmxMorph::group_offsets))
			.add_property("flip_offsets", make_getter(&PmxMorph::flip_offsets), make_setter(&PmxMorph::flip_offsets))
			.add_property("implus_offsets", make_getter(&PmxMorph::implus_offsets), make_setter(&PmxMorph::implus_offsets))
			;

		class_<PmxFrameElement>("PmxFrameElement")
			.add_property("element_target", make_getter(&PmxFrameElement::element_target), make_setter(&PmxFrameElement::element_target))
			.add_property("index", make_getter(&PmxFrameElement::index), make_setter(&PmxFrameElement::index))
			;

		make_vector<PmxFrameElement >();

		class_<PmxFrame>("PmxFrame")
			.add_property("frame_name", make_getter(&PmxFrame::frame_name), make_setter(&PmxFrame::frame_name))
			.add_property("frame_english_name", make_getter(&PmxFrame::frame_english_name), make_setter(&PmxFrame::frame_english_name))
			.add_property("frame_flag", make_getter(&PmxFrame::frame_flag), make_setter(&PmxFrame::frame_flag))
			.add_property("element_count", make_getter(&PmxFrame::element_count), make_setter(&PmxFrame::element_count))
			.add_property("elements", make_getter(&PmxFrame::elements), make_setter(&PmxFrame::elements))
			;

		class_<PmxRigidBody>("PmxRigidBody")
			.add_property("girid_body_name", make_getter(&PmxRigidBody::girid_body_name), make_setter(&PmxRigidBody::girid_body_name))
			.add_property("girid_body_english_name", make_getter(&PmxRigidBody::girid_body_english_name), make_setter(&PmxRigidBody::girid_body_english_name))
			.add_property("target_bone", make_getter(&PmxRigidBody::target_bone), make_setter(&PmxRigidBody::target_bone))
			.add_property("group", make_getter(&PmxRigidBody::group), make_setter(&PmxRigidBody::group))
			.add_property("mask", make_getter(&PmxRigidBody::mask), make_setter(&PmxRigidBody::mask))
			.add_property("shape", make_getter(&PmxRigidBody::shape), make_setter(&PmxRigidBody::shape))
			.add_property("size", make_array(&PmxRigidBody::size))
			.add_property("position", make_array(&PmxRigidBody::position))
			.add_property("orientation", make_array(&PmxRigidBody::orientation))
			.add_property("mass", make_getter(&PmxRigidBody::mass), make_setter(&PmxRigidBody::mass))
			.add_property("move_attenuation", make_getter(&PmxRigidBody::move_attenuation), make_setter(&PmxRigidBody::move_attenuation))
			.add_property("rotation_attenuation", make_getter(&PmxRigidBody::rotation_attenuation), make_setter(&PmxRigidBody::rotation_attenuation))
			.add_property("repulsion", make_getter(&PmxRigidBody::repulsion), make_setter(&PmxRigidBody::repulsion))
			.add_property("friction", make_getter(&PmxRigidBody::friction), make_setter(&PmxRigidBody::friction))
			.add_property("physics_calc_type", make_getter(&PmxRigidBody::physics_calc_type), make_setter(&PmxRigidBody::physics_calc_type))
			;

		enum_<PmxJointType>("PmxJointType")
			.value("Generic6DofSpring", PmxJointType::Generic6DofSpring)
			.value("Generic6Dof", PmxJointType::Generic6Dof)
			.value("Point2Point", PmxJointType::Point2Point)
			.value("ConeTwist", PmxJointType::ConeTwist)
			.value("Slider", PmxJointType::Slider)
			.value("Hinge", PmxJointType::Hinge)
			.export_values()
			;

		class_<PmxJointParam>("PmxJointParam")
			.add_property("rigid_body1", make_getter(&PmxJointParam::rigid_body1), make_setter(&PmxJointParam::rigid_body1))
			.add_property("rigid_body2", make_getter(&PmxJointParam::rigid_body2), make_setter(&PmxJointParam::rigid_body2))
			.add_property("position", make_array(&PmxJointParam::position))
			.add_property("orientaiton", make_array(&PmxJointParam::orientaiton))
			.add_property("move_limitation_min", make_array(&PmxJointParam::move_limitation_min))
			.add_property("move_limitation_max", make_array(&PmxJointParam::move_limitation_max))
			.add_property("rotation_limitation_min", make_array(&PmxJointParam::rotation_limitation_min))
			.add_property("rotation_limitation_max", make_array(&PmxJointParam::rotation_limitation_max))
			.add_property("spring_move_coefficient", make_array(&PmxJointParam::spring_move_coefficient))
			.add_property("spring_rotation_coefficient", make_array(&PmxJointParam::spring_rotation_coefficient))
			;

		class_<PmxJoint>("PmxJoint")
			.add_property("joint_name", make_getter(&PmxJoint::joint_name), make_setter(&PmxJoint::joint_name))
			.add_property("joint_english_name", make_getter(&PmxJoint::joint_english_name), make_setter(&PmxJoint::joint_english_name))
			.add_property("joint_type", make_getter(&PmxJoint::joint_type), make_setter(&PmxJoint::joint_type))
			.add_property("param", make_getter(&PmxJoint::param), make_setter(&PmxJoint::param))
			;

		enum_<PmxSoftBodyFlag>("PmxSoftBodyFlag")
			.value("BLink", PmxSoftBodyFlag::BLink)
			.value("Cluster", PmxSoftBodyFlag::Cluster)
			.value("Link", PmxSoftBodyFlag::Link)
			.export_values()
			;

		class_<PmxAncherRigidBody>("PmxAncherRigidBody")
			.add_property("related_rigid_body", make_getter(&PmxAncherRigidBody::related_rigid_body), make_setter(&PmxAncherRigidBody::related_rigid_body))
			.add_property("related_vertex", make_getter(&PmxAncherRigidBody::related_vertex), make_setter(&PmxAncherRigidBody::related_vertex))
			.add_property("is_near", make_getter(&PmxAncherRigidBody::is_near), make_setter(&PmxAncherRigidBody::is_near))
			;

		class_<PmxSoftBody>("PmxSoftBody");
		
		make_vector<PmxVertex>();
		make_vector<int>();
		make_vector<std::wstring>();
		make_vector<PmxMaterial>();
		make_vector<PmxBone>();
		make_vector<PmxMorph>();
		make_vector<PmxFrame>();
		make_vector<PmxRigidBody>();
		make_vector<PmxJoint>();
		make_vector<PmxSoftBody>();

		class_<PmxModel>("PmxModel")
			.add_property("version", make_getter(&PmxModel::version), make_setter(&PmxModel::version))
			.add_property("setting", make_getter(&PmxModel::setting), make_setter(&PmxModel::setting))
			.add_property("model_name", make_getter(&PmxModel::model_name), make_setter(&PmxModel::model_name))
			.add_property("model_english_name", make_getter(&PmxModel::model_english_name), make_setter(&PmxModel::model_english_name))
			.add_property("model_comment", make_getter(&PmxModel::model_comment), make_setter(&PmxModel::model_comment))
			.add_property("model_english_commnet", make_getter(&PmxModel::model_english_commnet), make_setter(&PmxModel::model_english_commnet))
			.add_property("vertex_count", make_getter(&PmxModel::vertex_count), make_setter(&PmxModel::vertex_count))
			.add_property("vertices", make_getter(&PmxModel::vertices))
			.add_property("index_count", make_getter(&PmxModel::index_count), make_setter(&PmxModel::index_count))
			.add_property("indices", make_getter(&PmxModel::indices))
			.add_property("texture_count", make_getter(&PmxModel::texture_count), make_setter(&PmxModel::texture_count))
			.add_property("textures", make_getter(&PmxModel::textures))
			.add_property("material_count", make_getter(&PmxModel::material_count), make_setter(&PmxModel::material_count))
			.add_property("materials", make_getter(&PmxModel::materials))
			.add_property("bone_count", make_getter(&PmxModel::bone_count), make_setter(&PmxModel::bone_count))
			.add_property("bones", make_getter(&PmxModel::bones))
			.add_property("morph_count", make_getter(&PmxModel::morph_count), make_setter(&PmxModel::morph_count))
			.add_property("morphs", make_getter(&PmxModel::morphs))
			.add_property("frame_count", make_getter(&PmxModel::frame_count), make_setter(&PmxModel::frame_count))
			.add_property("frames", make_getter(&PmxModel::frames))
			.add_property("rigid_body_count", make_getter(&PmxModel::rigid_body_count), make_setter(&PmxModel::rigid_body_count))
			.add_property("rigid_bodies", make_getter(&PmxModel::rigid_bodies))
			.add_property("joint_count", make_getter(&PmxModel::joint_count), make_setter(&PmxModel::joint_count))
			.add_property("joints", make_getter(&PmxModel::joints))
			.add_property("soft_body_count", make_getter(&PmxModel::soft_body_count), make_setter(&PmxModel::soft_body_count))
			.add_property("soft_bodies", make_getter(&PmxModel::soft_bodies))
			.def("init", &PmxModel::Init)
			.def("save_to_file", &save_pmx_to_file)
			;
}

}
