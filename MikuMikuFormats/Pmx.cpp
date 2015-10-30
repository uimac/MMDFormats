#include "Pmx.h"
#include "EncodingHelper.h"

namespace pmx
{
	/// インデックス値を読み込む
	int ReadIndex(std::istream *stream, int size)
	{
		switch (size)
		{
		case 1:
			uint8_t tmp8;
			stream->read((char*) &tmp8, sizeof(uint8_t));
			if (255 == tmp8)
			{
				return -1;
			}
			else {
				return (int) tmp8;
			}
		case 2:
			uint16_t tmp16;
			stream->read((char*) &tmp16, sizeof(uint16_t));
			if (65535 == tmp16)
			{
				return -1;
			}
			else {
				return (int) tmp16;
			}
		case 4:
			int tmp32;
			stream->read((char*) &tmp32, sizeof(int));
			return tmp32;
		default:
			return -1;
		}
	}

	/// インデックス値を書き込む
	void WriteIndex(std::ostream* stream, int index, int size)
	{
		if (size == 1)
		{
			if (index < 0) 
			{
				uint8_t value = 0xFF;
				stream->write((char*)&value, sizeof(uint8_t));
			}
			else
			{
				stream->write((char*)&index, sizeof(uint8_t));
			}
		}
		else if (size == 2)
		{
			if (index < 0)
			{
				uint16_t value = 0xFFFF;
				stream->write((char*)&value, sizeof(uint16_t));
			}
			else
			{
				stream->write((char*)&index, sizeof(uint16_t));
			}
		}
		else if (size == 4)
		{
			stream->write((char*)&index, sizeof(uint32_t));
		}
	}

	/// 文字列を読み込む
	std::wstring ReadString(std::istream *stream, uint8_t encoding)
	{
		oguna::EncodingConverter converter = oguna::EncodingConverter();
		int size;
		stream->read((char*) &size, sizeof(int));
		std::vector<char> buffer;
		if (size == 0)
		{
			return std::wstring(L"");
		}
		buffer.resize(size);
		stream->read((char*) buffer.data(), size);
		if (encoding == 0)
		{
			// UTF16
			return std::wstring((wchar_t*) buffer.data(), size / 2);
		}
		else
		{
			// UTF8
			std::wstring result;
			converter.Utf8ToUtf16(buffer.data(), size, &result);
			return result;
		}
	}

	/// 文字列を書き込む
	void WriteString(std::ostream *stream, std::wstring& str)
	{
		const int size = static_cast<int>(str.size() * 2);
		stream->write((char*)&size, sizeof(int));
		if (size > 0) {
			stream->write((char*)str.data(), size);
		}
	}

	void PmxSetting::Read(std::istream *stream)
	{
		uint8_t count;
		stream->read((char*) &count, sizeof(uint8_t));
		if (count < 8)
		{
			throw;
		}
		stream->read((char*) &encoding, sizeof(uint8_t));
		stream->read((char*) &uv, sizeof(uint8_t));
		stream->read((char*) &vertex_index_size, sizeof(uint8_t));
		stream->read((char*) &texture_index_size, sizeof(uint8_t));
		stream->read((char*) &material_index_size, sizeof(uint8_t));
		stream->read((char*) &bone_index_size, sizeof(uint8_t));
		stream->read((char*) &morph_index_size, sizeof(uint8_t));
		stream->read((char*) &rigidbody_index_size, sizeof(uint8_t));
		uint8_t temp;
		for (int i = 8; i < count; i++)
		{
			stream->read((char*)&temp, sizeof(uint8_t));
		}
	}

	void PmxSetting::Write(std::ostream *stream)
	{
		uint8_t count = 8;
		stream->write((char*)&count, sizeof(uint8_t));
		stream->write((char*)&encoding, sizeof(uint8_t));
		stream->write((char*)&uv, sizeof(uint8_t));
		stream->write((char*)&vertex_index_size, sizeof(uint8_t));
		stream->write((char*)&texture_index_size, sizeof(uint8_t));
		stream->write((char*)&material_index_size, sizeof(uint8_t));
		stream->write((char*)&bone_index_size, sizeof(uint8_t));
		stream->write((char*)&morph_index_size, sizeof(uint8_t));
		stream->write((char*)&rigidbody_index_size, sizeof(uint8_t));
	}

	void PmxVertexSkinningBDEF1::Read(std::istream *stream, PmxSetting *setting)
	{
		this->bone_index = ReadIndex(stream, setting->bone_index_size);
	}

	void PmxVertexSkinningBDEF1::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->bone_index, setting->bone_index_size);
	}

	void PmxVertexSkinningBDEF2::Read(std::istream *stream, PmxSetting *setting)
	{
		this->bone_index1 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index2 = ReadIndex(stream, setting->bone_index_size);
		stream->read((char*) &this->bone_weight, sizeof(float));
	}

	void PmxVertexSkinningBDEF2::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->bone_index1, setting->bone_index_size);
		WriteIndex(stream, this->bone_index2, setting->bone_index_size);
		stream->write((char*)&this->bone_weight, sizeof(float));
	}

	void PmxVertexSkinningBDEF4::Read(std::istream *stream, PmxSetting *setting)
	{
		this->bone_index1 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index2 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index3 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index4 = ReadIndex(stream, setting->bone_index_size);
		stream->read((char*) &this->bone_weight1, sizeof(float));
		stream->read((char*) &this->bone_weight2, sizeof(float));
		stream->read((char*) &this->bone_weight3, sizeof(float));
		stream->read((char*) &this->bone_weight4, sizeof(float));
	}

	void PmxVertexSkinningBDEF4::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->bone_index1, setting->bone_index_size);
		WriteIndex(stream, this->bone_index2, setting->bone_index_size);
		WriteIndex(stream, this->bone_index3, setting->bone_index_size);
		WriteIndex(stream, this->bone_index4, setting->bone_index_size);
		stream->write((char*)&this->bone_weight1, sizeof(float));
		stream->write((char*)&this->bone_weight2, sizeof(float));
		stream->write((char*)&this->bone_weight3, sizeof(float));
		stream->write((char*)&this->bone_weight4, sizeof(float));
	}

	void PmxVertexSkinningSDEF::Read(std::istream *stream, PmxSetting *setting)
	{
		this->bone_index1 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index2 = ReadIndex(stream, setting->bone_index_size);
		stream->read((char*) &this->bone_weight, sizeof(float));
		stream->read((char*) this->sdef_c, sizeof(float) * 3);
		stream->read((char*) this->sdef_r0, sizeof(float) * 3);
		stream->read((char*) this->sdef_r1, sizeof(float) * 3);
	}

	void PmxVertexSkinningSDEF::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->bone_index1, setting->bone_index_size);
		WriteIndex(stream, this->bone_index2, setting->bone_index_size);
		stream->write((char*)&this->bone_weight, sizeof(float));
		stream->write((char*) this->sdef_c, sizeof(float) * 3);
		stream->write((char*) this->sdef_r0, sizeof(float) * 3);
		stream->write((char*) this->sdef_r1, sizeof(float) * 3);
	}

	void PmxVertexSkinningQDEF::Read(std::istream *stream, PmxSetting *setting)
	{
		this->bone_index1 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index2 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index3 = ReadIndex(stream, setting->bone_index_size);
		this->bone_index4 = ReadIndex(stream, setting->bone_index_size);
		stream->read((char*) &this->bone_weight1, sizeof(float));
		stream->read((char*) &this->bone_weight2, sizeof(float));
		stream->read((char*) &this->bone_weight3, sizeof(float));
		stream->read((char*) &this->bone_weight4, sizeof(float));
	}

	void PmxVertexSkinningQDEF::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->bone_index1, setting->bone_index_size);
		WriteIndex(stream, this->bone_index2, setting->bone_index_size);
		WriteIndex(stream, this->bone_index3, setting->bone_index_size);
		WriteIndex(stream, this->bone_index4, setting->bone_index_size);
		stream->write((char*)&this->bone_weight1, sizeof(float));
		stream->write((char*)&this->bone_weight2, sizeof(float));
		stream->write((char*)&this->bone_weight3, sizeof(float));
		stream->write((char*)&this->bone_weight4, sizeof(float));
	}

	void PmxVertex::Read(std::istream *stream, PmxSetting *setting)
	{
		stream->read((char*) this->position, sizeof(float) * 3);
		stream->read((char*) this->normal, sizeof(float) * 3);
		stream->read((char*) this->uv, sizeof(float) * 2);
		for (int i = 0; i < setting->uv; ++i)
		{
			stream->read((char*) this->uva[i], sizeof(float) * 4);
		}
		stream->read((char*) &this->skinning_type, sizeof(PmxVertexSkinningType));
		switch (this->skinning_type)
		{
		case PmxVertexSkinningType::BDEF1:
			this->skinning = std::make_shared<PmxVertexSkinningBDEF1>();
			break;
		case PmxVertexSkinningType::BDEF2:
			this->skinning = std::make_shared<PmxVertexSkinningBDEF2>();
			break;
		case PmxVertexSkinningType::BDEF4:
			this->skinning = std::make_shared<PmxVertexSkinningBDEF4>();
			break;
		case PmxVertexSkinningType::SDEF:
			this->skinning = std::make_shared<PmxVertexSkinningSDEF>();
			break;
		case PmxVertexSkinningType::QDEF:
			this->skinning = std::make_shared<PmxVertexSkinningQDEF>();
			break;
		default:
			throw "invalid skinning type";
		}
		this->skinning->Read(stream, setting);
		stream->read((char*) &this->edge, sizeof(float));
	}

	void PmxVertex::Write(std::ostream *stream, PmxSetting *setting)
	{
		stream->write((char*) this->position, sizeof(float) * 3);
		stream->write((char*) this->normal, sizeof(float) * 3);
		stream->write((char*) this->uv, sizeof(float) * 2);
		for (int i = 0; i < setting->uv; ++i)
		{
			stream->write((char*) this->uva[i], sizeof(float) * 4);
		}
		stream->write((char*)&this->skinning_type, sizeof(uint8_t));
		this->skinning->Write(stream, setting);
		stream->write((char*)&this->edge, sizeof(float));
	}

	void PmxMaterial::Read(std::istream *stream, PmxSetting *setting)
	{
		this->material_name.swap(ReadString(stream, setting->encoding));
		this->material_english_name.swap(ReadString(stream, setting->encoding));
		stream->read((char*) this->diffuse, sizeof(float) * 4);
		stream->read((char*) this->specular, sizeof(float) * 3);
		stream->read((char*) &this->specularlity, sizeof(float));
		stream->read((char*) this->ambient, sizeof(float) * 3);
		stream->read((char*) &this->flag, sizeof(uint8_t));
		stream->read((char*) this->edge_color, sizeof(float) * 4);
		stream->read((char*) &this->edge_size, sizeof(float));
		this->diffuse_texture_index = ReadIndex(stream, setting->texture_index_size);
		this->sphere_texture_index = ReadIndex(stream, setting->texture_index_size);
		stream->read((char*) &this->sphere_op_mode, sizeof(uint8_t));
		stream->read((char*) &this->common_toon_flag, sizeof(uint8_t));
		if (this->common_toon_flag)
		{
			stream->read((char*) &this->toon_texture_index, sizeof(uint8_t));
		}
		else {
			this->toon_texture_index = ReadIndex(stream, setting->texture_index_size);
		}
		this->memo.swap(ReadString(stream, setting->encoding));
		stream->read((char*) &this->index_count, sizeof(int));
	}

	void PmxMaterial::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteString(stream, this->material_name);
		WriteString(stream, this->material_english_name);
		stream->write((char*) this->diffuse, sizeof(float) * 4);
		stream->write((char*) this->specular, sizeof(float) * 3);
		stream->write((char*)&this->specularlity, sizeof(float));
		stream->write((char*) this->ambient, sizeof(float) * 3);
		stream->write((char*)&this->flag, sizeof(uint8_t));
		stream->write((char*) this->edge_color, sizeof(float) * 4);
		stream->write((char*)&this->edge_size, sizeof(float));
		WriteIndex(stream, this->diffuse_texture_index, setting->texture_index_size);
		WriteIndex(stream, this->sphere_texture_index, setting->texture_index_size);
		stream->write((char*)&this->sphere_op_mode, sizeof(uint8_t));
		stream->write((char*)&this->common_toon_flag, sizeof(uint8_t));
		if (this->common_toon_flag)
		{
			stream->write((char*)&this->toon_texture_index, sizeof(uint8_t));
		}
		else {
			WriteIndex(stream, this->toon_texture_index, setting->texture_index_size);
		}
		WriteString(stream, this->memo);
		stream->write((char*)&this->index_count, sizeof(int));
	}

	void PmxIkLink::Read(std::istream *stream, PmxSetting *setting)
	{
		this->link_target = ReadIndex(stream, setting->bone_index_size);
		stream->read((char*) &this->angle_lock, sizeof(uint8_t));
		if (angle_lock == 1)
		{
			stream->read((char*) this->max_radian, sizeof(float) * 3);
			stream->read((char*) this->min_radian, sizeof(float) * 3);
		}
	}

	void PmxIkLink::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->link_target, setting->bone_index_size);
		stream->write((char*)&this->angle_lock, sizeof(uint8_t));
		if (this->angle_lock == 1)
		{
			stream->write((char*) this->max_radian, sizeof(float) * 3);
			stream->write((char*) this->min_radian, sizeof(float) * 3);
		}
	}

	void PmxBone::Read(std::istream *stream, PmxSetting *setting)
	{
		this->bone_name.swap(ReadString(stream, setting->encoding));
		this->bone_english_name.swap(ReadString(stream, setting->encoding));
		stream->read((char*) this->position, sizeof(float) * 3);
		this->parent_index = ReadIndex(stream, setting->bone_index_size);
		stream->read((char*) &this->level, sizeof(int));
		stream->read((char*) &this->bone_flag, sizeof(uint16_t));
		if (this->bone_flag & 0x0001) {
			this->target_index = ReadIndex(stream, setting->bone_index_size);
		}
		else {
			stream->read((char*)this->offset, sizeof(float) * 3);
		}
		if (this->bone_flag & (0x0100 | 0x0200)) {
			this->grant_parent_index = ReadIndex(stream, setting->bone_index_size);
			stream->read((char*) &this->grant_weight, sizeof(float));
		}
		if (this->bone_flag & 0x0400) {
			stream->read((char*)this->lock_axis_orientation, sizeof(float) * 3);
		}
		if (this->bone_flag & 0x0800) {
			stream->read((char*)this->local_axis_x_orientation, sizeof(float) * 3);
			stream->read((char*)this->local_axis_y_orientation, sizeof(float) * 3);
		}
		if (this->bone_flag & 0x2000) {
			stream->read((char*)this->key, sizeof(int));
		}
		if (this->bone_flag & 0x0020) {
			this->ik_target_bone_index = ReadIndex(stream, setting->bone_index_size);
			stream->read((char*) &ik_loop, sizeof(int));
			stream->read((char*) &ik_loop_angle_limit, sizeof(float));
			stream->read((char*) &ik_link_count, sizeof(int));
			this->ik_links.resize(ik_link_count);
			for (int i = 0; i < ik_link_count; i++) {
				ik_links[i].Read(stream, setting);
			}
		}
	}

	void PmxBone::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteString(stream, this->bone_name);
		WriteString(stream, this->bone_english_name);
		stream->write((char*) this->position, sizeof(float) * 3);
		WriteIndex(stream, this->parent_index, setting->bone_index_size);
		stream->write((char*)&this->level, sizeof(int));
		stream->write((char*)&this->bone_flag, sizeof(uint16_t));
		if (this->bone_flag & 0x0001) {
			WriteIndex(stream, this->target_index, setting->bone_index_size);
		}
		else {
			stream->write((char*)this->offset, sizeof(float) * 3);
		}
		if (this->bone_flag & (0x0100 | 0x0200)) {
			WriteIndex(stream, this->grant_parent_index, setting->bone_index_size);
			stream->write((char*)&this->grant_weight, sizeof(float));
		}
		if (this->bone_flag & 0x0400) {
			stream->write((char*)this->lock_axis_orientation, sizeof(float) * 3);
		}
		if (this->bone_flag & 0x0800) {
			stream->write((char*)this->local_axis_x_orientation, sizeof(float) * 3);
			stream->write((char*)this->local_axis_y_orientation, sizeof(float) * 3);
		}
		if (this->bone_flag & 0x2000) {
			stream->write((char*)this->key, sizeof(int));
		}
		if (this->bone_flag & 0x0020) {
			WriteIndex(stream, this->ik_target_bone_index, setting->bone_index_size);
			stream->write((char*)&ik_loop, sizeof(int));
			stream->write((char*)&ik_loop_angle_limit, sizeof(float));
			stream->write((char*)&ik_link_count, sizeof(int));
			for (int i = 0; i < ik_link_count; i++) {
				this->ik_links[i].Write(stream, setting);
			}
		}
	}

	void PmxMorphVertexOffset::Read(std::istream *stream, PmxSetting *setting)
	{
		this->vertex_index = ReadIndex(stream, setting->vertex_index_size);
		stream->read((char*)this->position_offset, sizeof(float) * 3);
	}

	void PmxMorphVertexOffset::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->vertex_index, setting->vertex_index_size);
		stream->write((char*)this->position_offset, sizeof(float) * 3);
	}

	void PmxMorphUVOffset::Read(std::istream *stream, PmxSetting *setting)
	{
		this->vertex_index = ReadIndex(stream, setting->vertex_index_size);
		stream->read((char*)this->uv_offset, sizeof(float) * 4);
	}

	void PmxMorphUVOffset::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->vertex_index, setting->vertex_index_size);
		stream->write((char*)this->uv_offset, sizeof(float) * 4);
	}

	void PmxMorphBoneOffset::Read(std::istream *stream, PmxSetting *setting)
	{
		this->bone_index = ReadIndex(stream, setting->bone_index_size);
		stream->read((char*)this->translation, sizeof(float) * 3);
		stream->read((char*)this->rotation, sizeof(float) * 4);
	}

	void PmxMorphBoneOffset::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->bone_index, setting->bone_index_size);
		stream->write((char*)this->translation, sizeof(float) * 3);
		stream->write((char*)this->rotation, sizeof(float) * 4);
	}

	void PmxMorphMaterialOffset::Read(std::istream *stream, PmxSetting *setting)
	{
		this->material_index = ReadIndex(stream, setting->material_index_size);
		stream->read((char*) &this->offset_operation, sizeof(uint8_t));
		stream->read((char*)this->diffuse, sizeof(float) * 4);
		stream->read((char*)this->specular, sizeof(float) * 3);
		stream->read((char*) &this->specularity, sizeof(float));
		stream->read((char*)this->ambient, sizeof(float) * 3);
		stream->read((char*)this->edge_color, sizeof(float) * 4);
		stream->read((char*) &this->edge_size, sizeof(float));
		stream->read((char*)this->texture_argb, sizeof(float) * 4);
		stream->read((char*)this->sphere_texture_argb, sizeof(float) * 4);
		stream->read((char*)this->toon_texture_argb, sizeof(float) * 4);
	}

	void PmxMorphMaterialOffset::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->material_index, setting->material_index_size);
		stream->write((char*)&this->offset_operation, sizeof(uint8_t));
		stream->write((char*)this->diffuse, sizeof(float) * 4);
		stream->write((char*)this->specular, sizeof(float) * 3);
		stream->write((char*)&this->specularity, sizeof(float));
		stream->write((char*)this->ambient, sizeof(float) * 3);
		stream->write((char*)this->edge_color, sizeof(float) * 4);
		stream->write((char*)&this->edge_size, sizeof(float));
		stream->write((char*)this->texture_argb, sizeof(float) * 4);
		stream->write((char*)this->sphere_texture_argb, sizeof(float) * 4);
		stream->write((char*)this->toon_texture_argb, sizeof(float) * 4);
	}

	void PmxMorphGroupOffset::Read(std::istream *stream, PmxSetting *setting)
	{
		this->morph_index = ReadIndex(stream, setting->morph_index_size);
		stream->read((char*) &this->morph_weight, sizeof(float));
	}

	void PmxMorphGroupOffset::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->morph_index, setting->morph_index_size);
		stream->write((char*)&this->morph_weight, sizeof(float));
	}

	void PmxMorphFlipOffset::Read(std::istream *stream, PmxSetting *setting)
	{
		this->morph_index = ReadIndex(stream, setting->morph_index_size);
		stream->read((char*) &this->morph_value, sizeof(float));
	}

	void PmxMorphFlipOffset::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->morph_index, setting->morph_index_size);
		stream->write((char*)&this->morph_value, sizeof(float));
	}

	void PmxMorphImplusOffset::Read(std::istream *stream, PmxSetting *setting)
	{
		this->rigid_body_index = ReadIndex(stream, setting->rigidbody_index_size);
		stream->read((char*) &this->is_local, sizeof(uint8_t));
		stream->read((char*)this->velocity, sizeof(float) * 3);
		stream->read((char*)this->angular_torque, sizeof(float) * 3);
	}

	void PmxMorphImplusOffset::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->rigid_body_index, setting->rigidbody_index_size);
		stream->write((char*)&this->is_local, sizeof(uint8_t));
		stream->write((char*)this->velocity, sizeof(float) * 3);
		stream->write((char*)this->angular_torque, sizeof(float) * 3);
	}

	void PmxMorph::Read(std::istream *stream, PmxSetting *setting)
	{
		this->morph_name = ReadString(stream, setting->encoding);
		this->morph_english_name = ReadString(stream, setting->encoding);
		stream->read((char*) &category, sizeof(MorphCategory));
		stream->read((char*) &morph_type, sizeof(MorphType));
		stream->read((char*) &this->offset_count, sizeof(int));
		switch (this->morph_type)
		{
		case MorphType::Group:
			group_offsets.resize(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				group_offsets[i].Read(stream, setting);
			}
			break;
		case MorphType::Vertex:
			vertex_offsets.resize(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				vertex_offsets[i].Read(stream, setting);
			}
			break;
		case MorphType::Bone:
			bone_offsets.resize(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				bone_offsets[i].Read(stream, setting);
			}
			break;
		case MorphType::Matrial:
			material_offsets.resize(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				material_offsets[i].Read(stream, setting);
			}
			break;
		case MorphType::UV:
		case MorphType::AdditionalUV1:
		case MorphType::AdditionalUV2:
		case MorphType::AdditionalUV3:
		case MorphType::AdditionalUV4:
			uv_offsets.resize(this->offset_count);
			for (int i = 0; i < offset_count; i++)
			{
				uv_offsets[i].Read(stream, setting);
			}
			break;
		default:
			throw;
		}
	}

	void PmxMorph::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteString(stream, this->morph_name);
		WriteString(stream, this->morph_english_name);
		stream->write((char*)&category, sizeof(MorphCategory));
		stream->write((char*)&morph_type, sizeof(MorphType));
		stream->write((char*)&this->offset_count, sizeof(int));
		switch (this->morph_type)
		{
		case MorphType::Group:
			for (int i = 0; i < offset_count; i++)
			{
				group_offsets[i].Write(stream, setting);
			}
			break;
		case MorphType::Vertex:
			for (int i = 0; i < offset_count; i++)
			{
				vertex_offsets[i].Write(stream, setting);
			}
			break;
		case MorphType::Bone:
			for (int i = 0; i < offset_count; i++)
			{
				bone_offsets[i].Write(stream, setting);
			}
			break;
		case MorphType::Matrial:
			for (int i = 0; i < offset_count; i++)
			{
				material_offsets[i].Write(stream, setting);
			}
			break;
		case MorphType::UV:
		case MorphType::AdditionalUV1:
		case MorphType::AdditionalUV2:
		case MorphType::AdditionalUV3:
		case MorphType::AdditionalUV4:
			for (int i = 0; i < offset_count; i++)
			{
				uv_offsets[i].Write(stream, setting);
			}
			break;
		default:
			throw;
		}
	}

	void PmxFrameElement::Read(std::istream *stream, PmxSetting *setting)
	{
		stream->read((char*) &this->element_target, sizeof(uint8_t));
		if (this->element_target == 0x00)
		{
			this->index = ReadIndex(stream, setting->bone_index_size);
		}
		else {
			this->index = ReadIndex(stream, setting->morph_index_size);
		}
	}

	void PmxFrameElement::Write(std::ostream *stream, PmxSetting *setting)
	{
		stream->write((char*)&this->element_target, sizeof(uint8_t));
		if (this->element_target == 0x00)
		{
			WriteIndex(stream, this->index, setting->bone_index_size);
		}
		else {
			WriteIndex(stream, this->index, setting->morph_index_size);
		}
	}

	void PmxFrame::Read(std::istream *stream, PmxSetting *setting)
	{
		this->frame_name = ReadString(stream, setting->encoding);
		this->frame_english_name = ReadString(stream, setting->encoding);
		stream->read((char*) &this->frame_flag, sizeof(uint8_t));
		stream->read((char*) &this->element_count, sizeof(int));
		this->elements.resize(this->element_count);
		for (int i = 0; i < this->element_count; i++)
		{
			this->elements[i].Read(stream, setting);
		}
	}

	void PmxFrame::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteString(stream, this->frame_name);
		WriteString(stream, this->frame_english_name);
		stream->write((char*)&this->frame_flag, sizeof(uint8_t));
		stream->write((char*)&this->element_count, sizeof(int));
		for (int i = 0; i < this->element_count; i++)
		{
			this->elements[i].Write(stream, setting);
		}
	}

	void PmxRigidBody::Read(std::istream *stream, PmxSetting *setting)
	{
		this->girid_body_name = ReadString(stream, setting->encoding);
		this->girid_body_english_name = ReadString(stream, setting->encoding);
		this->target_bone = ReadIndex(stream, setting->bone_index_size);
		stream->read((char*) &this->group, sizeof(uint8_t));
		stream->read((char*) &this->mask, sizeof(uint16_t));
		stream->read((char*) &this->shape, sizeof(uint8_t));
		stream->read((char*) this->size, sizeof(float) * 3);
		stream->read((char*) this->position, sizeof(float) * 3);
		stream->read((char*) this->orientation, sizeof(float) * 3);
		stream->read((char*) &this->mass, sizeof(float));
		stream->read((char*) &this->move_attenuation, sizeof(float));
		stream->read((char*) &this->rotation_attenuation, sizeof(float));
		stream->read((char*) &this->repulsion, sizeof(float));
		stream->read((char*) &this->friction, sizeof(float));
		stream->read((char*) &this->physics_calc_type, sizeof(uint8_t));
	}

	void PmxRigidBody::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteString(stream, this->girid_body_name);
		WriteString(stream, this->girid_body_english_name);
		WriteIndex(stream, this->target_bone, setting->bone_index_size);
		stream->write((char*)&this->group, sizeof(uint8_t));
		stream->write((char*)&this->mask, sizeof(uint16_t));
		stream->write((char*)&this->shape, sizeof(uint8_t));
		stream->write((char*) this->size, sizeof(float) * 3);
		stream->write((char*) this->position, sizeof(float) * 3);
		stream->write((char*) this->orientation, sizeof(float) * 3);
		stream->write((char*)&this->mass, sizeof(float));
		stream->write((char*)&this->move_attenuation, sizeof(float));
		stream->write((char*)&this->rotation_attenuation, sizeof(float));
		stream->write((char*)&this->repulsion, sizeof(float));
		stream->write((char*)&this->friction, sizeof(float));
		stream->write((char*)&this->physics_calc_type, sizeof(uint8_t));
	}

	void PmxJointParam::Read(std::istream *stream, PmxSetting *setting)
	{
		this->rigid_body1 = ReadIndex(stream, setting->rigidbody_index_size);
		this->rigid_body2 = ReadIndex(stream, setting->rigidbody_index_size);
		stream->read((char*) this->position, sizeof(float) * 3);
		stream->read((char*) this->orientaiton, sizeof(float) * 3);
		stream->read((char*) this->move_limitation_min, sizeof(float) * 3);
		stream->read((char*) this->move_limitation_max, sizeof(float) * 3);
		stream->read((char*) this->rotation_limitation_min, sizeof(float) * 3);
		stream->read((char*) this->rotation_limitation_max, sizeof(float) * 3);
		stream->read((char*) this->spring_move_coefficient, sizeof(float) * 3);
		stream->read((char*) this->spring_rotation_coefficient, sizeof(float) * 3);
	}

	void PmxJointParam::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->rigid_body1, setting->rigidbody_index_size);
		WriteIndex(stream, this->rigid_body2, setting->rigidbody_index_size);
		stream->write((char*) this->position, sizeof(float) * 3);
		stream->write((char*) this->orientaiton, sizeof(float) * 3);
		stream->write((char*) this->move_limitation_min, sizeof(float) * 3);
		stream->write((char*) this->move_limitation_max, sizeof(float) * 3);
		stream->write((char*) this->rotation_limitation_min, sizeof(float) * 3);
		stream->write((char*) this->rotation_limitation_max, sizeof(float) * 3);
		stream->write((char*) this->spring_move_coefficient, sizeof(float) * 3);
		stream->write((char*) this->spring_rotation_coefficient, sizeof(float) * 3);
	}

	void PmxJoint::Read(std::istream *stream, PmxSetting *setting)
	{
		this->joint_name = ReadString(stream, setting->encoding);
		this->joint_english_name = ReadString(stream, setting->encoding);
		stream->read((char*) &this->joint_type, sizeof(uint8_t));
		this->param.Read(stream, setting);
	}

	void PmxJoint::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteString(stream, this->joint_name);
		WriteString(stream, this->joint_english_name);
		stream->write((char*)&this->joint_type, sizeof(uint8_t));
		this->param.Write(stream, setting);
	}

	void PmxAncherRigidBody::Read(std::istream *stream, PmxSetting *setting)
	{
		this->related_rigid_body = ReadIndex(stream, setting->rigidbody_index_size);
		this->related_vertex = ReadIndex(stream, setting->vertex_index_size);
		stream->read((char*) &this->is_near, sizeof(uint8_t));
	}

	void PmxAncherRigidBody::Write(std::ostream *stream, PmxSetting *setting)
	{
		WriteIndex(stream, this->related_rigid_body, setting->rigidbody_index_size);
		WriteIndex(stream, this->related_vertex, setting->vertex_index_size);
		stream->write((char*)&this->is_near, sizeof(uint8_t));
	}

	void PmxSoftBody::Read(std::istream *stream, PmxSetting *setting)
	{
		// 未実装
		std::cerr << "Not Implemented Exception" << std::endl;
		throw;
	}

	void PmxSoftBody::Write(std::ostream *stream, PmxSetting *setting)
	{
		// 未実装
		std::cerr << "Not Implemented Exception" << std::endl;
		throw;
	}

	void PmxModel::Init()
	{
		this->version = 0.0f;
		this->model_name.clear();
		this->model_english_name.clear();
		this->model_comment.clear();
		this->model_english_commnet.clear();
		this->vertex_count = 0;
		this->vertices.clear();
		this->index_count = 0;
		this->indices.clear();
		this->texture_count = 0;
		this->textures.clear();
		this->material_count = 0;
		this->materials.clear();
		this->bone_count = 0;
		this->bones.clear();
		this->morph_count = 0;
		this->morphs.clear();
		this->frame_count = 0;
		this->frames.clear();
		this->rigid_body_count = 0;
		this->rigid_bodies.clear();
		this->joint_count = 0;
		this->joints.clear();
		this->soft_body_count = 0;
		this->soft_bodies.clear();
	}

	void PmxModel::Read(std::istream *stream)
	{
		// マジック
		char magic[4];
		stream->read((char*) magic, sizeof(char) * 4);
		if (magic[0] != 0x50 || magic[1] != 0x4d || magic[2] != 0x58 || magic[3] != 0x20)
		{
			std::cerr << "invalid magic number." << std::endl;
			throw;
		}
		// バージョン
		stream->read((char*) &version, sizeof(float));
		if (version != 2.0f && version != 2.1f)
		{
			std::cerr << "this is not ver2.0 or ver2.1 but " << version << "." << std::endl;
			throw;
		}
		// ファイル設定
		this->setting.Read(stream);

		// モデル情報
		this->model_name.swap(ReadString(stream, setting.encoding));
		this->model_english_name.swap(ReadString(stream, setting.encoding));
		this->model_comment.swap(ReadString(stream, setting.encoding));
		this->model_english_commnet.swap(ReadString(stream, setting.encoding));

		// 頂点
		stream->read((char*) &vertex_count, sizeof(int));
		this->vertices.resize(vertex_count);
		for (int i = 0; i < vertex_count; i++)
		{
			vertices[i].Read(stream, &setting);
		}

		// 面
		stream->read((char*) &index_count, sizeof(int));
		this->indices.resize(index_count);
		for (int i = 0; i < index_count; i++)
		{
			this->indices[i] = ReadIndex(stream, setting.vertex_index_size);
		}

		// テクスチャ
		stream->read((char*) &texture_count, sizeof(int));
		this->textures.resize(texture_count);
		for (int i = 0; i < texture_count; i++)
		{
			this->textures[i] = ReadString(stream, setting.encoding);
		}

		// マテリアル
		stream->read((char*) &material_count, sizeof(int));
		this->materials.resize(material_count);
		for (int i = 0; i < material_count; i++)
		{
			this->materials[i].Read(stream, &setting);
		}

		// ボーン
		stream->read((char*) &this->bone_count, sizeof(int));
		this->bones.resize(this->bone_count);
		for (int i = 0; i < this->bone_count; i++)
		{
			this->bones[i].Read(stream, &setting);
		}

		// モーフ
		stream->read((char*) &this->morph_count, sizeof(int));
		this->morphs.resize(this->morph_count);
		for (int i = 0; i < this->morph_count; i++)
		{
			this->morphs[i].Read(stream, &setting);
		}

		// 表示枠
		stream->read((char*) &this->frame_count, sizeof(int));
		this->frames.resize(this->frame_count);
		for (int i = 0; i < this->frame_count; i++)
		{
			this->frames[i].Read(stream, &setting);
		}

		// 剛体
		stream->read((char*) &this->rigid_body_count, sizeof(int));
		this->rigid_bodies.resize(this->rigid_body_count);
		for (int i = 0; i < this->rigid_body_count; i++)
		{
			this->rigid_bodies[i].Read(stream, &setting);
		}

		// ジョイント
		stream->read((char*) &this->joint_count, sizeof(int));
		this->joints.resize(this->joint_count);
		for (int i = 0; i < this->joint_count; i++)
		{
			this->joints[i].Read(stream, &setting);
		}

		//// ソフトボディ
		//if (this->version == 2.1f)
		//{
		//	stream->read((char*) &this->soft_body_count, sizeof(int));
		//	this->soft_bodies = std::make_unique<PmxSoftBody []>(this->soft_body_count);
		//	for (int i = 0; i < this->soft_body_count; i++)
		//	{
		//		this->soft_bodies[i].Read(stream, &setting);
		//	}
		//}
	}

	void PmxModel::Write(std::ostream *stream)
	{
		// マジック
		char magic[4] = {
			0x50, 0x4d, 0x58, 0x20
		};
		stream->write((char*)magic, sizeof(char) * 4);

		float version = 2.0f;
		// バージョン
		stream->write((char*)&version, sizeof(float));
		// ファイル設定
		this->setting.Write(stream);

		// モデル情報
		WriteString(stream, this->model_name);
		WriteString(stream, this->model_english_name);
		WriteString(stream, this->model_comment);
		WriteString(stream, this->model_english_commnet);

		// 頂点
		stream->write((char*)&vertex_count, sizeof(int));
		for (int i = 0; i < vertex_count; i++)
		{
			vertices[i].Write(stream, &setting);
		}

		// 面
		stream->write((char*)&index_count, sizeof(int));
		for (int i = 0; i < index_count; i++)
		{
			WriteIndex(stream, this->indices[i], setting.vertex_index_size);
		}

		// テクスチャ
		stream->write((char*)&texture_count, sizeof(int));
		for (int i = 0; i < texture_count; i++)
		{
			WriteString(stream, this->textures[i]);
		}

		// マテリアル
		stream->write((char*)&material_count, sizeof(int));
		for (int i = 0; i < material_count; i++)
		{
			this->materials[i].Write(stream, &setting);
		}

		// ボーン
		stream->write((char*)&this->bone_count, sizeof(int));
		for (int i = 0; i < this->bone_count; i++)
		{
			this->bones[i].Write(stream, &setting);
		}

		// モーフ
		stream->write((char*)&this->morph_count, sizeof(int));
		for (int i = 0; i < this->morph_count; i++)
		{
			this->morphs[i].Write(stream, &setting);
		}

		// 表示枠
		stream->write((char*)&this->frame_count, sizeof(int));
		for (int i = 0; i < this->frame_count; i++)
		{
			this->frames[i].Write(stream, &setting);
		}

		// 剛体
		stream->write((char*)&this->rigid_body_count, sizeof(int));
		for (int i = 0; i < this->rigid_body_count; i++)
		{
			this->rigid_bodies[i].Write(stream, &setting);
		}

		// ジョイント
		stream->write((char*)&this->joint_count, sizeof(int));
		for (int i = 0; i < this->joint_count; i++)
		{
			this->joints[i].Write(stream, &setting);
		}
	}

	//std::unique_ptr<PmxModel> ReadFromFile(const char *filename)
	//{
	//	auto stream = std::ifstream(filename, std::ios_base::binary);
	//	auto pmx = PmxModel::ReadFromStream(&stream);
	//	if (!stream.eof())
	//	{
	//		std::cerr << "don't reach the end of file." << std::endl;
	//	}
	//	stream.close();
	//	return pmx;
	//}

	//std::unique_ptr<PmxModel> ReadFromStream(std::istream *stream)
	//{
	//	auto pmx = std::make_unique<PmxModel>();
	//	pmx->Read(stream);
	//	return pmx;
	//}
}
