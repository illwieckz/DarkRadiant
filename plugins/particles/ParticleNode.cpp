#include "ParticleNode.h"

#include "ivolumetest.h"
#include "itextstream.h"

namespace particles
{

ParticleNode::ParticleNode(const RenderableParticlePtr& particle) :
	_renderableParticle(particle),
	_local2Parent(Matrix4::getIdentity())
{}

IRenderableParticlePtr ParticleNode::getParticle() const
{
	return _renderableParticle;
}

const AABB& ParticleNode::localAABB() const
{
	return _renderableParticle->getBounds();
}

bool ParticleNode::isHighlighted(void) const
{
	return false;
}

const Matrix4& ParticleNode::localToParent() const
{
	scene::INodePtr parent = getParent();

	if (parent == NULL)
	{
		_local2Parent = Matrix4::getIdentity();
	}
	else
	{
		_local2Parent = parent->localToWorld();

		// compensate the parent rotation only
		_local2Parent.t().x() = 0;
		_local2Parent.t().y() = 0;
		_local2Parent.t().z() = 0;

		_local2Parent.invert();
	}

	return _local2Parent;
}

void ParticleNode::renderSolid(RenderableCollector& collector, 
							   const VolumeTest& volume) const
{
	if (!_renderableParticle) return;

	// Update the particle system before rendering
	update(volume);

	_renderableParticle->renderSolid(collector, volume, localToWorld(), _renderEntity.get());
}

void ParticleNode::renderWireframe(RenderableCollector& collector, 
								   const VolumeTest& volume) const
{
	if (!_renderableParticle) return;

	// Update the particle system before rendering
	update(volume);

	_renderableParticle->renderWireframe(collector, volume, localToWorld(), _renderEntity.get());
}

void ParticleNode::onInsertIntoScene()
{
	scene::Node::onInsertIntoScene();

	if (_renderEntity)
	{
		_renderEntity->setRequiredShaderFlags(_renderEntity->getRequiredShaderFlags() | RENDER_COLOURARRAY);
	}
}

void ParticleNode::onRemoveFromScene()
{
	scene::Node::onRemoveFromScene();

	if (_renderEntity)
	{
		_renderEntity->setRequiredShaderFlags(_renderEntity->getRequiredShaderFlags() & ~RENDER_COLOURARRAY);
	}
}

void ParticleNode::update(const VolumeTest& viewVolume) const
{
	// Get the view rotation and cancel out the translation part
	Matrix4 viewRotation = viewVolume.GetModelview();
	viewRotation.t() = Vector4(0,0,0,1);

	// Get the main direction of our parent entity
	_renderableParticle->setMainDirection(_renderEntity->getDirection());

	// Set entity colour, might be needed
	_renderableParticle->setEntityColour(Vector3(
		_renderEntity->getShaderParm(0), _renderEntity->getShaderParm(1), _renderEntity->getShaderParm(2)));

	_renderableParticle->update(GlobalRenderSystem().getTime(), 
		GlobalRenderSystem(), viewRotation);
}

} // namespace