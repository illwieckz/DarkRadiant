#include "RadiantTest.h"

#include "ieclass.h"
#include "ientity.h"
#include "irendersystemfactory.h"
#include "iselectable.h"
#include "iselection.h"
#include "ishaders.h"

#include "render/NopVolumeTest.h"
#include "string/convert.h"
#include "transformlib.h"

namespace test
{

using EntityTest = RadiantTest;

namespace
{

// Create an entity from a simple classname string
IEntityNodePtr createByClassName(const std::string& className)
{
    auto cls = GlobalEntityClassManager().findClass(className);
    return GlobalEntityModule().createEntity(cls);
}

// Obtain entity attachments as a simple std::list
std::list<Entity::Attachment> getAttachments(const IEntityNodePtr node)
{
    std::list<Entity::Attachment> attachments;
    if (node)
    {
        node->getEntity().forEachAttachment(
            [&](const Entity::Attachment& a) { attachments.push_back(a); }
        );
    }
    return attachments;
}

}

using StringMap = std::map<std::string, std::string>;

TEST_F(EntityTest, LookupEntityClass)
{
    // Nonexistent class should return null (but not throw or crash)
    auto cls = GlobalEntityClassManager().findClass("notAnEntityClass");
    EXPECT_FALSE(cls);

    // Real entity class should return a valid pointer
    auto lightCls = GlobalEntityClassManager().findClass("light");
    EXPECT_TRUE(lightCls);
}

TEST_F(EntityTest, LightEntitiesRecognisedAsLights)
{
    // The 'light' class should be recognised as an actual light
    auto lightCls = GlobalEntityClassManager().findClass("light");
    EXPECT_TRUE(lightCls->isLight());

    // Things which are not lights should also be correctly identified
    auto notLightCls = GlobalEntityClassManager().findClass("dr:entity_using_modeldef");
    EXPECT_TRUE(notLightCls);
    EXPECT_FALSE(notLightCls->isLight());

    // Anything deriving from the light class should also be a light
    auto derived1 = GlobalEntityClassManager().findClass("atdm:light_base");
    EXPECT_TRUE(derived1->isLight());

    // Second level derivations too
    auto derived2 = GlobalEntityClassManager().findClass("light_extinguishable");
    EXPECT_TRUE(derived2->isLight());

    // torch_brazier is not a light itself, but has a light attached, so it
    // should not have isLight() == true
    auto brazier = GlobalEntityClassManager().findClass("atdm:torch_brazier");
    EXPECT_FALSE(brazier->isLight());
}

TEST_F(EntityTest, CannotCreateEntityWithoutClass)
{
    // Creating with a null entity class should throw an exception
    EXPECT_THROW(GlobalEntityModule().createEntity({}), std::runtime_error);
}

TEST_F(EntityTest, CreateBasicLightEntity)
{
    // Create a basic light
    auto lightCls = GlobalEntityClassManager().findClass("light");
    auto light = GlobalEntityModule().createEntity(lightCls);

    // Light has a sensible autogenerated name
    EXPECT_EQ(light->name(), "light_1");

    // Entity should have a "classname" key matching the actual entity class we
    // created
    auto clsName = light->getEntity().getKeyValue("classname");
    EXPECT_EQ(clsName, "light");

    // Entity should have an IEntityClass pointer which matches the one we
    // looked up
    EXPECT_EQ(light->getEntity().getEntityClass().get(), lightCls.get());

    // This basic light entity should have no attachments
    auto attachments = getAttachments(light);
    EXPECT_EQ(attachments.size(), 0);
}

TEST_F(EntityTest, EnumerateEntitySpawnargs)
{
    auto light = createByClassName("light");
    auto& spawnArgs = light->getEntity();

    // Visit spawnargs by key and value string
    StringMap keyValuesInit;
    spawnArgs.forEachKeyValue([&](const std::string& k, const std::string& v) {
        keyValuesInit.insert({k, v});
    });

    // Initial entity should have a name and a classname value and no other
    // properties
    EXPECT_EQ(keyValuesInit.size(), 2);
    EXPECT_EQ(keyValuesInit["name"], light->name());
    EXPECT_EQ(keyValuesInit["classname"], "light");

    // Add some new properties of our own
    spawnArgs.setKeyValue("origin", "128 256 -1024");
    spawnArgs.setKeyValue("_color", "0.5 0.5 0.5");

    // Ensure that our new properties are also enumerated
    StringMap keyValuesAll;
    spawnArgs.forEachKeyValue([&](const std::string& k, const std::string& v) {
        keyValuesAll.insert({k, v});
    });
    EXPECT_EQ(keyValuesAll.size(), 4);
    EXPECT_EQ(keyValuesAll["origin"], "128 256 -1024");
    EXPECT_EQ(keyValuesAll["_color"], "0.5 0.5 0.5");

    // Enumerate as full EntityKeyValue objects as well as strings
    StringMap keyValuesByObj;
    spawnArgs.forEachEntityKeyValue(
        [&](const std::string& k, const EntityKeyValue& v) {
            keyValuesByObj.insert({k, v.get()});
        }
    );
    EXPECT_EQ(keyValuesAll, keyValuesByObj);
}

TEST_F(EntityTest, EnumerateInheritedSpawnargs)
{
    auto light = createByClassName("atdm:light_base");
    auto& spawnArgs = light->getEntity();

    // Enumerate all keyvalues including the inherited ones
    StringMap keyValues;
    spawnArgs.forEachKeyValue(
        [&](const std::string& k, const std::string& v) {
            keyValues.insert({k, v});
        },
        true /* includeInherited */
    );

    // Check we have some inherited properties from the entitydef (including
    // spawnclass from the entitydef's own parent def)
    EXPECT_EQ(keyValues["spawnclass"], "idLight");
    EXPECT_EQ(keyValues["shouldBeOn"], "0");
    EXPECT_EQ(keyValues["AIUse"], "AIUSE_LIGHTSOURCE");
    EXPECT_EQ(keyValues["noshadows"], "0");
}

TEST_F(EntityTest, GetKeyValuePairs)
{
    auto torch = createByClassName("atdm:torch_brazier");
    auto& spawnArgs = torch->getEntity();

    using Pair = Entity::KeyValuePairs::value_type;

    // Retrieve single spawnargs as single-element lists of pairs
    auto classNamePairs = spawnArgs.getKeyValuePairs("classname");
    EXPECT_EQ(classNamePairs.size(), 1);
    EXPECT_EQ(classNamePairs[0], Pair("classname", "atdm:torch_brazier"));

    auto namePairs = spawnArgs.getKeyValuePairs("name");
    EXPECT_EQ(namePairs.size(), 1);
    EXPECT_EQ(namePairs[0], Pair("name", "atdm_torch_brazier_1"));

    // Add some spawnargs with a common prefix
    const StringMap SR_KEYS{
        {"sr_type_1", "blah"},
        {"sr_type_2", "bleh"},
        {"sR_tYpE_a", "123"},
        {"SR_type_1a", "0 123 -120"},
    };
    for (const auto& pair: SR_KEYS)
        spawnArgs.setKeyValue(pair.first, pair.second);

    // Confirm all added prefix keys are found regardless of case
    auto srPairs = spawnArgs.getKeyValuePairs("sr_type");
    EXPECT_EQ(srPairs.size(), SR_KEYS.size());
    for (const auto& pair: srPairs)
        EXPECT_EQ(SR_KEYS.at(pair.first), pair.second);
}

TEST_F(EntityTest, CopySpawnargs)
{
    auto light = createByClassName("atdm:light_base");
    auto& spawnArgs = light->getEntity();

    // Add some custom spawnargs to copy
    const StringMap EXTRA_SPAWNARGS{{"first", "1"},
                                    {"second", "two"},
                                    {"THIRD", "3333"},
                                    {"_color", "1 0 1"}};

    for (const auto& pair: EXTRA_SPAWNARGS)
        spawnArgs.setKeyValue(pair.first, pair.second);

    // Clone the entity node
    auto lightCopy = light->clone();
    Entity* clonedEnt = Node_getEntity(lightCopy);
    ASSERT_TRUE(clonedEnt);

    // Clone should have all the same spawnarg strings
    std::size_t count = 0;
    clonedEnt->forEachKeyValue([&](const std::string& k, const std::string& v) {
        EXPECT_EQ(spawnArgs.getKeyValue(k), v);
        ++count;
    });
    EXPECT_EQ(count, EXTRA_SPAWNARGS.size() + 2 /* name and classname */);

    // Clone should NOT have the same actual KeyValue object pointers, although
    // the count should be the same
    std::set<EntityKeyValue*> origPointers;
    std::set<EntityKeyValue*> copiedPointers;
    spawnArgs.forEachEntityKeyValue(
        [&](const std::string& k, EntityKeyValue& v) {
            origPointers.insert(&v);
        });
    clonedEnt->forEachEntityKeyValue(
        [&](const std::string& k, EntityKeyValue& v) {
            copiedPointers.insert(&v);
        });
    EXPECT_EQ(origPointers.size(), count);
    EXPECT_EQ(copiedPointers.size(), count);

    std::vector<EntityKeyValue*> overlap;
    std::set_intersection(origPointers.begin(), origPointers.end(),
                          copiedPointers.begin(), copiedPointers.end(),
                          std::back_inserter(overlap));
    EXPECT_EQ(overlap.size(), 0);
}

TEST_F(EntityTest, SelectEntity)
{
    auto light = createByClassName("light");

    // Confirm that setting entity node's selection status propagates to the
    // selection system
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0);
    Node_getSelectable(light)->setSelected(true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);
    Node_getSelectable(light)->setSelected(false);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0);
}

TEST_F(EntityTest, DestroySelectedEntity)
{
    auto light = createByClassName("light");

    // Confirm that setting entity node's selection status propagates to the
    // selection system
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0);
    Node_getSelectable(light)->setSelected(true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);

    // Destructor called here and should not crash
}

namespace
{
    // A simple RenderableCollector which just logs/stores whatever is submitted
    struct TestRenderableCollector: public RenderableCollector
    {
        // Count of submitted renderables and lights
        int renderables = 0;
        int lights = 0;

        // List of actual RendererLight objects
        std::list<const RendererLight*> lightPtrs;

        void addRenderable(Shader& shader, const OpenGLRenderable& renderable,
                           const Matrix4& localToWorld,
                           const LitObject* litObject = nullptr,
                           const IRenderEntity* entity = nullptr) override
        {
            ++renderables;
        }

        void addLight(const RendererLight& light)
        {
            ++lights;
            lightPtrs.push_back(&light);
        }

        bool supportsFullMaterials() const override { return true; }
        void setHighlightFlag(Highlight::Flags flags, bool enabled) override
        {}
    };

    // Collection of objects needed for rendering. Since not all tests require
    // rendering, these objects are in an auxiliary fixture created when needed
    // rather than part of the EntityTest fixture used by every test. This class
    // also implements scene::NodeVisitor enabling it to visit trees of nodes
    // for rendering.
    struct RenderFixture: public scene::NodeVisitor
    {
        RenderSystemPtr backend = GlobalRenderSystemFactory().createRenderSystem();
        render::NopVolumeTest volumeTest;
        TestRenderableCollector collector;

        // Whether to render solid or wireframe
        const bool renderSolid;

        // Keep track of nodes visited
        int nodesVisited = 0;

        // Construct
        RenderFixture(bool solid = false): renderSolid(solid)
        {}

        // Convenience method to set render backend and traverse a node and its
        // children for rendering
        void renderSubGraph(const scene::INodePtr& node)
        {
            node->setRenderSystem(backend);
            node->traverse(*this);
        }

        // NodeVisitor implementation
        bool pre(const scene::INodePtr& node) override
        {
            // Count the node itself
            ++nodesVisited;

            // Render the node in appropriate mode
            if (renderSolid)
                node->renderSolid(collector, volumeTest);
            else
                node->renderWireframe(collector, volumeTest);

            // Continue traversing
            return true;
        }
    };
}

TEST_F(EntityTest, ModifyEntityClass)
{
    auto cls = GlobalEntityClassManager().findClass("light");
    auto light = GlobalEntityModule().createEntity(cls);
    auto& spawnArgs = light->getEntity();

    // Light doesn't initially have a colour set
    RenderFixture rf;
    light->setRenderSystem(rf.backend);
    const ShaderPtr origWireShader = light->getWireShader();
    ASSERT_TRUE(origWireShader);

    // The shader shouldn't just change by itself (this would invalidate the
    // test)
    EXPECT_EQ(light->getWireShader(), origWireShader);

    // Set a new colour value on the entity *class* (not the entity)
    cls->setColour(Vector3(0.5, 0.24, 0.87));

    // Shader should have changed due to the entity class update (although there
    // aren't currently any public Shader properties that we can examine to
    // confirm its contents)
    EXPECT_NE(light->getWireShader(), origWireShader);
}

TEST_F(EntityTest, LightLocalToWorldFromOrigin)
{
    auto light = createByClassName("light");

    // Initial localToWorld should be identity
    EXPECT_EQ(light->localToWorld(), Matrix4::getIdentity());

    // Set an origin
    const Vector3 ORIGIN(123, 456, -10);
    light->getEntity().setKeyValue("origin", string::to_string(ORIGIN));

    // localToParent should reflect the new origin
    auto transformNode = std::dynamic_pointer_cast<ITransformNode>(light);
    ASSERT_TRUE(transformNode);
    EXPECT_EQ(transformNode->localToParent(), Matrix4::getTranslation(ORIGIN));

    // Since there is no parent, the final localToWorld should be the same as
    // localToParent
    EXPECT_EQ(light->localToWorld(), Matrix4::getTranslation(ORIGIN));
}

TEST_F(EntityTest, LightTransformedByParent)
{
    // Parent a light to another entity (this isn't currently how the attachment
    // system is implemented, but it should validate that a light node can
    // inherit the transformation of its parent).
    auto light = createByClassName("light");
    auto parentModel = createByClassName("func_static");
    parentModel->addChildNode(light);

    // Parenting should automatically set the parent pointer of the child
    EXPECT_EQ(light->getParent(), parentModel);

    // Set an offset for the parent model
    const Vector3 ORIGIN(1024, 512, -320);
    parentModel->getEntity().setKeyValue("origin", string::to_string(ORIGIN));

    // Parent entity should have a transform matrix corresponding to its
    // translation
    EXPECT_EQ(parentModel->localToWorld(), Matrix4::getTranslation(ORIGIN));

    // The light itself should have the same transformation as the parent (since
    // the method is localToWorld not localToParent).
    EXPECT_EQ(light->localToWorld(), Matrix4::getTranslation(ORIGIN));

    // Render the light to obtain the RendererLight pointer
    RenderFixture renderF(true /* solid */);
    renderF.renderSubGraph(parentModel);
    EXPECT_EQ(renderF.nodesVisited, 2);
    EXPECT_EQ(renderF.collector.lights, 1);
    ASSERT_FALSE(renderF.collector.lightPtrs.empty());

    // Check the rendered light's geometry
    const RendererLight* rLight = renderF.collector.lightPtrs.front();
    EXPECT_EQ(rLight->getLightOrigin(), ORIGIN);
    EXPECT_EQ(rLight->lightAABB().origin, ORIGIN);
    EXPECT_EQ(rLight->lightAABB().extents, Vector3(320, 320, 320));
}

TEST_F(EntityTest, RenderUnselectedLightEntity)
{
    auto light = createByClassName("light");
    RenderFixture renderF;

    // Render the light in wireframe mode.
    light->setRenderSystem(renderF.backend);
    light->renderWireframe(renderF.collector, renderF.volumeTest);

    // Only the light origin diamond should be rendered
    EXPECT_EQ(renderF.collector.renderables, 1);
    EXPECT_EQ(renderF.collector.lights, 0);
}

TEST_F(EntityTest, RenderSelectedLightEntity)
{
    auto light = createByClassName("light");
    RenderFixture renderF;

    // Select the light then render it in wireframe mode
    Node_getSelectable(light)->setSelected(true);
    light->setRenderSystem(renderF.backend);
    light->renderWireframe(renderF.collector, renderF.volumeTest);

    // With the light selected, we should get the origin diamond, the radius and
    // the center vertex.
    EXPECT_EQ(renderF.collector.renderables, 3);
    EXPECT_EQ(renderF.collector.lights, 0);
}

TEST_F(EntityTest, RenderLightAsLightSource)
{
    auto light = createByClassName("light_torchflame_small");
    auto& spawnArgs = light->getEntity();

    // Set a non-default origin for the light
    static const Vector3 ORIGIN(-64, 128, 963);
    spawnArgs.setKeyValue("origin", string::to_string(ORIGIN));

    // Render the light in full materials mode
    RenderFixture renderF;
    light->setRenderSystem(renderF.backend);
    light->renderSolid(renderF.collector, renderF.volumeTest);

    // We should get one renderable for the origin diamond, and one light source
    EXPECT_EQ(renderF.collector.renderables, 1);
    EXPECT_EQ(renderF.collector.lights, 1);

    // Confirm properties of the submitted RendererLight
    ASSERT_EQ(renderF.collector.lightPtrs.size(), 1);
    const RendererLight* rLight = renderF.collector.lightPtrs.front();
    ASSERT_TRUE(rLight);
    EXPECT_EQ(rLight->getLightOrigin(), ORIGIN);
    EXPECT_EQ(rLight->lightAABB().origin, ORIGIN);

    // Default light properties from the entitydef
    EXPECT_EQ(rLight->lightAABB().extents, Vector3(240, 240, 240));
    ASSERT_TRUE(rLight->getShader() && rLight->getShader()->getMaterial());
    EXPECT_EQ(rLight->getShader()->getMaterial()->getName(),
              "lights/biground_torchflicker");
}

TEST_F(EntityTest, RenderEmptyFuncStatic)
{
    auto funcStatic = createByClassName("func_static");

    // Func static without a model key is empty
    RenderFixture rf;
    rf.renderSubGraph(funcStatic);
    EXPECT_EQ(rf.nodesVisited, 1);
    EXPECT_EQ(rf.collector.lights, 0);
    EXPECT_EQ(rf.collector.renderables, 0);
}

TEST_F(EntityTest, RenderFuncStaticWithModel)
{
    // Create a func_static with a model key
    auto funcStatic = createByClassName("func_static");
    funcStatic->getEntity().setKeyValue("model", "models/moss_patch.ase");

    RenderFixture rf;
    rf.renderSubGraph(funcStatic);

    // The entity node itself does not render the model; it is a parent node
    // with the model as a child (e.g. as a StaticModelNode). Therefore we
    // should have visited two nodes in total: the entity and its model child.
    EXPECT_EQ(rf.nodesVisited, 2);

    // Only one of the nodes should have submitted renderables
    EXPECT_EQ(rf.collector.lights, 0);
    EXPECT_EQ(rf.collector.renderables, 1);
}

TEST_F(EntityTest, RenderFuncStaticWithMultiSurfaceModel)
{
    // Create a func_static with a model key
    auto funcStatic = createByClassName("func_static");
    funcStatic->getEntity().setKeyValue("model", "models/torch.lwo");

    // This torch model has 3 renderable surfaces
    RenderFixture rf;
    rf.renderSubGraph(funcStatic);
    EXPECT_EQ(rf.collector.lights, 0);
    EXPECT_EQ(rf.collector.renderables, 3);
}

TEST_F(EntityTest, CreateAttachedLightEntity)
{
    // Create the torch entity which has an attached light
    auto torch = createByClassName("atdm:torch_brazier");
    ASSERT_TRUE(torch);

    // Check that the attachment spawnargs are present
    const Entity& spawnArgs = torch->getEntity();
    EXPECT_EQ(spawnArgs.getKeyValue("def_attach"), "light_cageflame_small");
    EXPECT_EQ(spawnArgs.getKeyValue("pos_attach"), "flame");
    EXPECT_EQ(spawnArgs.getKeyValue("name_attach"), "flame");

    // Spawnargs should be parsed into a single attachment
    auto attachments = getAttachments(torch);
    EXPECT_EQ(attachments.size(), 1);

    // Examine the properties of the single attachment
    Entity::Attachment attachment = attachments.front();
    EXPECT_EQ(attachment.eclass, "light_cageflame_small");
    EXPECT_EQ(attachment.offset, Vector3(0, 0, 10));
}

TEST_F(EntityTest, RenderAttachedLightEntity)
{
    auto torch = createByClassName("atdm:torch_brazier");
    ASSERT_TRUE(torch);

    // Confirm that def has the right model
    auto& spawnArgs = torch->getEntity();
    EXPECT_EQ(spawnArgs.getKeyValue("model"), "models/torch.lwo");

    // We must render in solid mode to get the light source
    RenderFixture rf(true /* solid mode */);
    rf.renderSubGraph(torch);

    // The node visitor should have visited the entity itself and one child node (a
    // static model)
    EXPECT_EQ(rf.nodesVisited, 2);

    // There should be 3 renderables from the torch (because the entity has a
    // shadowmesh and a collision mesh as well as the main model) and one from
    // the light (the origin diamond).
    EXPECT_EQ(rf.collector.renderables, 4);

    // The attached light should have been submitted as a light source
    EXPECT_EQ(rf.collector.lights, 1);

    // The submitted light should be fully realised with a light shader
    const RendererLight* rLight = rf.collector.lightPtrs.front();
    ASSERT_TRUE(rLight);
    EXPECT_TRUE(rLight->getShader());
}

TEST_F(EntityTest, AttachedLightAtCorrectPosition)
{
    const Vector3 ORIGIN(256, -128, 635);
    const Vector3 EXPECTED_OFFSET(0, 0, 10); // attach offset in def

    // Create a torch node and set a non-zero origin
    auto torch = createByClassName("atdm:torch_brazier");
    torch->getEntity().setKeyValue("origin", string::to_string(ORIGIN));

    // Render the torch
    RenderFixture rf(true /* solid mode */);
    rf.renderSubGraph(torch);

    // Access the submitted light source
    ASSERT_FALSE(rf.collector.lightPtrs.empty());
    const RendererLight* rLight = rf.collector.lightPtrs.front();
    ASSERT_TRUE(rLight);

    // Check the light source's position
    EXPECT_EQ(rLight->getLightOrigin(), ORIGIN + EXPECTED_OFFSET);
    EXPECT_EQ(rLight->lightAABB().origin, ORIGIN + EXPECTED_OFFSET);
}

}