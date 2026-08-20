import unreal

def prop(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as e:
        return '<ERR %s>' % e

def safe(obj, name):
    try:
        return getattr(obj, name)
    except Exception as e:
        return '<ERR %s>' % e

def describe_graph(graph):
    unreal.log('GRAPH object=%s type=%s' % (graph, type(graph)))
    if isinstance(graph, str):
        return
    unreal.log('GRAPH %s class=%s' % (graph.get_name(), graph.get_class().get_name()))
    nodes = prop(graph, 'nodes')
    unreal.log('  nodes=%d' % (len(nodes) if nodes else 0))
    for node in nodes or []:
        unreal.log('  NODE name=%s class=%s title=%s' % (node.get_name(), node.get_class().get_name(), safe(node, 'node_title')))
        for pname in ('function_reference', 'member_reference', 'event_reference', 'custom_function_name', 'function_name', 'variable_reference'):
            value = prop(node, pname)
            if value not in (None, '', '<ERR %s>' % value):
                unreal.log('    %s=%s' % (pname, value))
        pins = prop(node, 'pins')
        for pin in pins or []:
            linked = []
            for other in (prop(pin, 'linked_to') or []):
                linked.append('%s.%s' % (safe(other, 'get_outer')() if hasattr(other, 'get_outer') else '?', prop(other, 'pin_name')))
            unreal.log('    PIN %s dir=%s type=%s sub=%s default=%s links=%s' % (prop(pin, 'pin_name'), prop(pin, 'direction'), prop(pin, 'pin_type'), prop(pin, 'pin_sub_category'), prop(pin, 'default_value'), linked))

asset = unreal.load_asset('/Game/Magic/GameplayCues/GCN_LightBeam')
unreal.log('ASSET %s class=%s' % (asset, asset.get_class().get_name() if asset else None))
if asset:
    try:
        unreal.load_module('BlueprintEditorLibrary')
        event_graph = unreal.BlueprintEditorLibrary.find_event_graph(asset)
        unreal.log('EVENT_GRAPH %s' % event_graph)
        if event_graph:
            describe_graph(event_graph)
    except Exception as e:
        unreal.log('BLUEPRINT_LIB_ERR %s' % e)

    generated = unreal.load_object(None, '/Game/Magic/GameplayCues/GCN_LightBeam.GCN_LightBeam_C')
    unreal.log('GENERATED %s' % generated)
    if generated:
        cdo = unreal.get_default_object(generated)
        unreal.log('CDO %s class=%s' % (cdo, cdo.get_class().get_name()))
        for pname in ('gameplay_cue_tag', 'default_placement_info', 'burst_effects', 'default_spawn_condition', 'auto_destroy_delay'):
            unreal.log('CDO_PROP %s=%s' % (pname, prop(cdo, pname)))

    try:
        for graph in unreal.get_objects_of_class(unreal.EdGraph, True):
            if '/Game/Magic/GameplayCues/GCN_LightBeam' in str(graph.get_outer()):
                describe_graph(graph)
    except Exception as e:
        unreal.log('GRAPH_SCAN_ERR %s' % e)
