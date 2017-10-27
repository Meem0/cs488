rootnode = gr.node('root')
rootnode:rotate('y', -20.0)
rootnode:translate(0.0, 0.0, -1.0)

red = gr.material({1.0, 0.0, 0.0}, {0.1, 0.1, 0.1}, 10)
blue = gr.material({0.0, 0.0, 1.0}, {0.1, 0.1, 0.1}, 10)
green = gr.material({0.0, 1.0, 0.0}, {0.1, 0.1, 0.1}, 10)
white = gr.material({1.0, 1.0, 1.0}, {0.1, 0.1, 0.1}, 10)

torso = gr.mesh('cube', 'torso')
rootnode:add_child(torso)
torso:set_material(white)

neckJoint = gr.joint('neckJoint', {-30.0, 0.0, 30.0}, {-30.0, 0.0, 30.0})
torso:add_child(neckJoint)
--neckJoint:translate(0.0, 1.0, 0.0)
neckJoint:rotate('z', 90.0)

head = gr.mesh('cube', 'head')
neckJoint:add_child(head)
head:translate(0.0, 0.5, 0.0)
--head:rotate('z', -90.0)
head:set_material(red)

return rootnode
