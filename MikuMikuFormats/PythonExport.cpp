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

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <wchar.h>

#include "Vmd.h"

namespace
{

	using namespace boost::python;
	using namespace vmd;


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

	bool save_to_file(VmdMotion& motion, const char* filepath)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
		std::u16string utf16str = convert.from_bytes(filepath);
		return motion.SaveToFile(utf16str);
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

BOOST_PYTHON_MODULE(mmformat)
{
		class_<VmdBoneFrame>("VmdBoneFrame")
			.add_property("name", make_getter(&VmdBoneFrame::name), make_setter(&VmdBoneFrame::name))
			.add_property("frame", make_getter(&VmdBoneFrame::frame), make_setter(&VmdBoneFrame::frame))
			.add_property("position", &get_bone_frame_position, &set_bone_frame_position)
			.add_property("orientation", &get_bone_frame_orientation, &set_bone_frame_orientation)
			//.add_property("interpolation", make_getter(&VmdBoneFrame::interpolation), make_setter(&VmdBoneFrame::interpolation))
		;

		class_<VmdMotion>("VmdMotion")
			.add_property("model_name", make_getter(&VmdMotion::model_name), make_setter(&VmdMotion::model_name))
			.def("add_bone_frame", &add_bone_frame)
			.def("save_to_file", &save_to_file)
			;
}

}
