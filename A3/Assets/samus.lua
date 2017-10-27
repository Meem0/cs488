function mat(r, g, b)
  return gr.material({r, g, b}, {0.1, 0.1, 0.1}, 10)
end

--red = gr.material({0.831, 0.251, 0.294}, {0.1, 0.1, 0.1}, 10)
--orange = gr.material({0.9, 0.580, 0.31}, {0.1, 0.1, 0.1}, 10)
--green = gr.material({0.8, 1.0, 0.66}, {0.1, 0.1, 0.1}, 10)
--yellow = gr.material({0.953, 0.918, 0.306}, {0.1, 0.1, 0.1}, 10)
--blue = gr.material({0.184, 0.557, 0.58}, {0.1, 0.1, 0.1}, 10)
red = mat(0.831, 0.251, 0.294)
orange = mat(0.9, 0.580, 0.31)
--green = mat(0.8, 1.0, 0.66)
green = gr.material({0.8, 1.0, 0.66}, {0.01, 0.9, 0.01}, 500)
yellow = mat(0.953, 0.918, 0.306)
blue = gr.material({0.184, 0.557, 0.58}, {0.1, 0.1, 0.9}, 500)
black = mat(0.294, 0.286, 0.271)
zeroVec = {0.0, 0.0, 0.0}

function get_inv(vec)
	return {1.0 / vec[1], 1.0 / vec[2], 1.0 / vec[3]}
end

function set_scale(node, vec)
	node:scale(vec[1], vec[2], vec[3])
end

function inv_scale_node(name, parent, sc)
  node = gr.joint(name, zeroVec, zeroVec)
  set_scale(node, get_inv(sc))
  parent:add_child(node)
  return node
end

function upper_arm(parent, side_name, dir)
  local shoulderJoint = gr.joint(
    'shoulder_' .. side_name .. '_joint',
	--{-20.0, 0.0, 20.0},
	zeroVec,
	{dir > 0 and -5.0 or -120.0, 0.0, dir < 0 and 5.0 or 120.0}
  )
  parent:add_child(shoulderJoint)
  shoulderJoint:rotate('z', dir * -90)
  shoulderJoint:rotate('y', dir * -90)
  shoulderJoint:translate(dir * 2.0, 0.6, 0.0)
  
  local shoulder = gr.mesh('sphere', 'shoulder_' .. side_name)
  shoulderJoint:add_child(shoulder)
  shoulderScs = {1.12, 1.12, 1.12}
  set_scale(shoulder, shoulderScs)
  shoulder:rotate('y', dir * 90)
  shoulder:rotate('z', dir * 90)
  shoulder:set_material(orange)
  
  local shoulderInv = inv_scale_node('shoulder_' .. side_name .. '_inv', shoulder, shoulderScs)
  
  local shoulderDecor = gr.mesh('sphere', 'shoulder_decor_' .. side_name)
  shoulderInv:add_child(shoulderDecor)
  shoulderDecor:scale(0.3, 0.3, 0.1)
  shoulderDecor:rotate('y', dir * -5.0)
  shoulderDecor:translate(dir * -0.13, -0.0, 1.05)
  shoulderDecor:set_material(green)
  
  local armJoint = gr.joint(
    'arm_' .. side_name .. '_joint',
	{dir > 0 and -70.0 or -50.0, 0.0, dir > 0 and 50.0 or 70.0},
	{-80.0, 0.0, 180.0}
  )
  shoulderInv:add_child(armJoint)
  armJoint:rotate('z', 90)
  
  local arm = gr.mesh('sphere', 'arm_' .. side_name)
  armJoint:add_child(arm)
  armScs = {0.57, 1.05, 0.57}
  set_scale(arm, armScs)
  arm:translate(dir * 0.4, -1.7, 0.0)
  arm:rotate('z', -90)
  arm:set_material(orange)
  
  local armInv = inv_scale_node('arm_' .. side_name .. '_inv', arm, armScs)
  
  local forearmJoint = gr.joint('forearm_' .. side_name .. '_joint', zeroVec, {0.0, 0.0, 130.0})
  armInv:add_child(forearmJoint)
  forearmJoint:rotate('z', 90)
  forearmJoint:translate(0.0, -0.8, 0.0)
  
  return forearmJoint
end

function leg(parent, side_name, dir)
  local thighSocket = gr.mesh('sphere', 'thigh_socket_' .. side_name)
  parent:add_child(thighSocket)
  thighSocketSc = {0.75, 0.3, 0.75}
  set_scale(thighSocket, thighSocketSc)
  thighSocket:rotate('z', dir * 65.0)
  thighSocket:translate(dir * 0.65, -0.65, 0.0)
  thighSocket:set_material(black)
  
  thighSocketInv = gr.joint('thigh_socket_' .. side_name .. '_inv', zeroVec, zeroVec)
  thighSocket:add_child(thighSocketInv)
  thighSocketInv:rotate('z', dir * -65.0)
  set_scale(thighSocketInv, get_inv(thighSocketSc))
  
  local thighJoint = gr.joint(
    'thigh_' .. side_name .. '_joint',
	{dir < 0 and -5.0 or -50.0, 0.0, dir < 0 and 50.0 or 5.0},
	{-70.0, 0.0, 70.0}
  )
  thighSocketInv:add_child(thighJoint)
  thighJoint:rotate('y', 90.0)
  thighJoint:rotate('z', 90.0)
  --thighJoint:translate(0.0, -1.5, 0.0)
  
  local thigh = gr.mesh('sphere', 'thigh_' .. side_name)
  thighJoint:add_child(thigh)
  thighSc = {0.68, 1.9, 0.68}
  set_scale(thigh, thighSc)
  thigh:translate(dir * 0.46, -1.4, 0.0)
  thigh:rotate('z', -90.0)
  thigh:rotate('y', -90.0)
  thigh:set_material(orange)
  
  local thighInv = inv_scale_node('thigh_' .. side_name .. '_inv', thigh, thighSc)
  
  local calfJoint = gr.joint('calf_' .. side_name .. '_joint', zeroVec, {-130.0, 0.0, 0.0})
  thighInv:add_child(calfJoint)
  calfJoint:rotate('z', 90)
  calfJoint:translate(0.0, -1.0, 0.0)
  
  local calf = gr.mesh('sphere', 'calf_' .. side_name)
  calfJoint:add_child(calf)
  calfSc = {0.6, 1.9, 0.6}
  set_scale(calf, calfSc)
  calf:translate(0.0, -2.1, 0.0)
  calf:rotate('z', -90)
  calf:set_material(orange)
  
  local calfInv = inv_scale_node('calf_' .. side_name .. '_inv', calf, calfSc)
  
  local footJoint = gr.joint('foot_' .. side_name .. '_joint', zeroVec, {-30.0, 0.0, 30.0})
  calfInv:add_child(footJoint)
  footJoint:rotate('z', 90)
  footJoint:translate(0.0, -0.9, 0.0)
  
  local foot = gr.mesh('sphere', 'foot_' .. side_name)
  footJoint:add_child(foot)
  footSc = {0.45, 0.45, 0.9}
  set_scale(foot, footSc)
  foot:translate(0.0, -0.7, 0.5)
  foot:rotate('z', -90)
  foot:set_material(orange)
  
  return foot
end

rootnode = gr.node('root')
rootnode:scale(0.25, 0.25, 0.25)
rootnode:rotate('y', -20.0)
rootnode:translate(0.0, 0.0, -1.0)

torsoLow = gr.mesh('sphere', 'torso_low')
rootnode:add_child(torsoLow)
torsoLowScs = {1.0, 1.4, 1.0}
set_scale(torsoLow, torsoLowScs)
torsoLow:set_material(yellow)

torsoLowInv = inv_scale_node('torso_low_inv', torsoLow, torsoLowScs)

local torsoLowJoint = gr.joint('torso_low_joint', zeroVec, {-30.0, 0.0, 30.0})
torsoLowInv:add_child(torsoLowJoint)

torsoMid = gr.mesh('sphere', 'torso_mid')
torsoLowJoint:add_child(torsoMid)
torsoMidScs = {1.3, 1.2, 1.1}
set_scale(torsoMid, torsoMidScs)
torsoMid:translate(0.0, 2.0, 0.0)
torsoMid:set_material(yellow)

torsoMidInv = inv_scale_node('torso_mid_inv', torsoMid, torsoMidScs)

torsoUp = gr.mesh('sphere', 'torso_up')
torsoMidInv:add_child(torsoUp)
torsoUpScs = {1.8, 0.9, 1.4}
set_scale(torsoUp, torsoUpScs)
torsoUp:translate(0.0, 0.5, 0.0)
torsoUp:set_material(red)

torsoUpInv = inv_scale_node('torso_up_inv', torsoUp, torsoUpScs)

neckSocket = gr.mesh('sphere', 'neck_socket')
torsoUpInv:add_child(neckSocket)
neckSocketScs = {0.9, 0.5, 0.9}
set_scale(neckSocket, neckSocketScs)
neckSocket:translate(0.0, 0.55, 0.0)
neckSocket:set_material(black)

torsoUpInv = inv_scale_node('torso_up_inv', torsoUp, torsoUpScs)

neckJoint = gr.joint('neckJoint', {-70.0, 0.0, 70.0}, {-20.0, 0.0, 30.0})
torsoUpInv:add_child(neckJoint)
neckJoint:rotate('z', 90.0)
neckJoint:translate(0.0, 1.5, 0.0)

head = gr.mesh('sphere', 'head')
neckJoint:add_child(head)
headScs = {0.95, 0.95, 0.95}
set_scale(head, headScs)
head:translate(0.0, 0.2, 0.0)
head:rotate('z', -90.0)
head:set_material(red)

headInv = inv_scale_node('head_inv', head, headScs)

visorLeft = gr.mesh('cube', 'visor_left')
headInv:add_child(visorLeft)
visorLeft:scale(0.8, 0.5, 0.3)
visorLeft:rotate('z', 5.0)
visorLeft:translate(0.25, -0.06, 0.85)
visorLeft:set_material(green)

visorRight = gr.mesh('cube', 'visor_right')
headInv:add_child(visorRight)
visorRight:scale(0.8, 0.5, 0.3)
visorRight:rotate('z', -5.0)
visorRight:translate(-0.25, -0.06, 0.85)
visorRight:set_material(green)

visorBottom = gr.mesh('cube', 'visor_bottom')
headInv:add_child(visorBottom)
visorBottom:scale(0.65, 0.3, 0.3)
visorBottom:translate(0.0, -0.35, 0.85)
visorBottom:set_material(green)

leftArm = upper_arm(torsoUpInv, 'left', 1.0)
rightArm = upper_arm(torsoUpInv, 'right', -1.0)

leftForearm = gr.mesh('sphere', 'left_forearm')
leftArm:add_child(leftForearm)
leftForearmScs = {0.45, 0.8, 0.45}
set_scale(leftForearm, leftForearmScs)
leftForearm:translate(0.0, -0.7, 0.0)
leftForearm:rotate('z', -90.0)
leftForearm:set_material(orange)

leftForearmInv = inv_scale_node('left_forearm_inv', leftForearm, leftForearmScs)

handJoint = gr.joint('hand_joint', zeroVec, {-40.0, 0.0, 130.0})
leftForearmInv:add_child(handJoint)
handJoint:rotate('z', 90)
handJoint:translate(0.0, -0.8, 0.0)

leftHand = gr.mesh('sphere', 'left_hand')
handJoint:add_child(leftHand)
leftHand:scale(0.35, 0.5, 0.2)
leftHand:translate(0.0, -0.3, 0.0)
leftHand:rotate('z', -90)
leftHand:set_material(orange)

cannon = gr.mesh('sphere', 'cannon')
rightArm:add_child(cannon)
cannonScs = {0.6, 1.6, 0.6}
set_scale(cannon, cannonScs)
cannon:translate(0.0, -0.9, 0.0)
cannon:rotate('z', -90.0)
cannon:set_material(blue)

cannonInv = inv_scale_node('cannon_inv', cannon, cannonScs)

canonUpper = gr.mesh('cube', 'canon_upper')
cannonInv:add_child(canonUpper)
canonUpperScs = {1.2, 1.2, 0.9}
set_scale(canonUpper, canonUpperScs)
canonUpper:translate(0.0, 0.2, 0.0)
canonUpper:set_material(blue)

canonUpperInv = inv_scale_node('cannon_upper_inv', canonUpper, canonUpperScs)

canonDecor1 = gr.mesh('sphere', 'canon_decor_1')
canonUpperInv:add_child(canonDecor1)
canonDecor1:scale(0.03, 0.6, 0.1)
canonDecor1:translate(-0.6, 0, 0.0)
canonDecor1:set_material(green)

canonDecor2 = gr.mesh('sphere', 'canon_decor_2')
canonUpperInv:add_child(canonDecor2)
canonDecor2:scale(0.03, 0.6, 0.1)
canonDecor2:translate(0.6, 0, 0.0)
canonDecor2:set_material(green)

--canonTip = gr.mesh('cube', 'canon_tip')
--cannonInv:add_child(canonTip)
--canonTipScs = {0.5, 0.5, 0.5}
--set_scale(canonTip, canonTipScs)
--canonTip:translate(0.0, -1.4, 0.0)
--canonTip:set_material(blue)
--
--canonTipInv = inv_scale_node('canon_tip_inv', canonTip, canonTipScs)

leftFoot = leg(torsoLowInv, 'left', 1.0)
rightFoot = leg(torsoLowInv, 'right', -1.0)

return rootnode
