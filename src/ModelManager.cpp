#include "ModelManager.h"
#include "GLTFLoader.h"

void ModelManager::LoadModel(const std::filesystem::path& path)
{
	CommandContext uploadContext;
	uploadContext.InitializeCommandContext(QUEUETYPE::QUEUE_UPLOAD);

	std::shared_ptr<Model> model;

	bool constructedModel = GLTFLoader::ConstructModelFromFile(path, model, uploadContext.GetCommandList());

	if (constructedModel)
	{
		uploadContext.KeepAlive(model);
		model->RegisterWithGUI();
		model->AppendDrawItems(_drawItems);

		_models.push_back(std::move(model));
	}

	uploadContext.Finish(true);
}

void ModelManager::ComputeGlobalTransforms()
{
	for (auto& model : _models)
		model->ComputeGlobalTransforms();
}

void ModelManager::DrawAll(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext, const XMFLOAT4X4& viewMatrix)
{
	std::erase_if(_drawItems, [](const Model::DrawItem& item) { return item.model.expired(); });
	for (Model::DrawItem& item : _drawItems)
	{
		if (const std::shared_ptr<Model> model = item.model.lock())
		{
			commandContext->KeepAlive(model);
			model->UpdateDrawItem(item, viewMatrix);
		}
	}

	if (shaderPass->_transparentPipelineState)
	{
		const std::vector<Model::DrawItem>::iterator blendedBegin = std::partition(_drawItems.begin(), _drawItems.end(), [](const Model::DrawItem& item)
		{
			return item.alphaMode != fastgltf::AlphaMode::Blend;
		});

		// order by aabb center
		std::sort(blendedBegin, _drawItems.end(), [](const Model::DrawItem& a, const Model::DrawItem& b)
		{
			return a.viewDepth > b.viewDepth;
		});
	}

	commandContext->SetPipelineState(shaderPass->_pipelineState);
	bool blending = false;
	for (const Model::DrawItem& item : _drawItems)
	{
		const std::shared_ptr<Model> model = item.model.lock();
		if (!model) continue;
		if (item.alphaMode == fastgltf::AlphaMode::Blend)
		{
			if (!shaderPass->_transparentPipelineState)
				continue;
			if (!blending)
			{
				commandContext->SetPipelineState(shaderPass->_transparentPipelineState);
				blending = true;
			}
		}
		model->DrawPrimitive(item, shaderPass, commandContext->GetCommandList());
	}
}

void ModelManager::DrawAllBoundingBoxes(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext)
{
	for (auto& model : _models)
	{
		commandContext->KeepAlive(model);
		model->DrawModelBoundingBox(shaderPass, commandContext->GetCommandList());
	}
}
void ModelManager::ClearModels()
{
	// Recorded/submitted contexts retain their own model owners until completion.
	_drawItems.clear();
	_models.clear();
}
