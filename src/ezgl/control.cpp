
#include "ezgl/control.hpp"

#include "ezgl/camera.hpp"
#include "ezgl/canvas.hpp"
#include "globals.h"

namespace ezgl {

static rectangle zoom_in_world(point2d zoom_point, rectangle world, double zoom_factor)
{
  double const left = zoom_point.x - (zoom_point.x - world.left()) / zoom_factor;
  double const bottom = zoom_point.y + (world.bottom() - zoom_point.y) / zoom_factor;

  double const right = zoom_point.x + (world.right() - zoom_point.x) / zoom_factor;
  double const top = zoom_point.y - (zoom_point.y - world.top()) / zoom_factor;

  return {{left, bottom}, {right, top}};
}

static rectangle zoom_out_world(point2d zoom_point, rectangle world, double zoom_factor)
{
  double const left = zoom_point.x - (zoom_point.x - world.left()) * zoom_factor;
  double const bottom = zoom_point.y + (world.bottom() - zoom_point.y) * zoom_factor;

  double const right = zoom_point.x + (world.right() - zoom_point.x) * zoom_factor;
  double const top = zoom_point.y - (zoom_point.y - world.top()) * zoom_factor;

  return {{left, bottom}, {right, top}};
}

void zoom_in(canvas *cnv, double zoom_factor)
{
  point2d const zoom_point = cnv->get_camera().get_world().center();
  rectangle const world = cnv->get_camera().get_world();

  cnv->get_camera().set_world(zoom_in_world(zoom_point, world, zoom_factor));
  cnv->redraw();
}

void zoom_in(canvas *cnv, point2d zoom_point, double zoom_factor)
{
  zoom_point = cnv->get_camera().widget_to_world(zoom_point);
  rectangle const world = cnv->get_camera().get_world();

  cnv->get_camera().set_world(zoom_in_world(zoom_point, world, zoom_factor));
  cnv->redraw();
}

void zoom_out(canvas *cnv, double zoom_factor)
{
  point2d const zoom_point = cnv->get_camera().get_world().center();
  rectangle const world = cnv->get_camera().get_world();

  cnv->get_camera().set_world(zoom_out_world(zoom_point, world, zoom_factor));
  cnv->redraw();
}

void zoom_out(canvas *cnv, point2d zoom_point, double zoom_factor)
{
  zoom_point = cnv->get_camera().widget_to_world(zoom_point);
  rectangle const world = cnv->get_camera().get_world();

  cnv->get_camera().set_world(zoom_out_world(zoom_point, world, zoom_factor));
  cnv->redraw();
}

void zoom_fit(canvas *cnv, rectangle region)
{
  rectangle vis_screen = cnv->get_camera().get_widget();
  rectangle world = cnv->get_camera().get_initial_world();
  point2d center = cnv->get_camera().get_initial_world().center();

  double screen_width = vis_screen.right() - vis_screen.left();
  double screen_height = vis_screen.top() - vis_screen.bottom();
  double world_width = world.right() - world.left();
  double world_height = world.top() - world.bottom();

  double screen_ratio = screen_width/screen_height;
  double world_ratio = world_width/world_height;

  double left, right, top, bottom, size_factor = 0;

  if (screen_ratio > 1 || world_ratio <= 1){
    size_factor = 1/screen_ratio;
  }
  else{
    size_factor = screen_ratio;
  }

  if (size_factor > 0.88){
    size_factor = 0.6/size_factor;
  }

  left = center.x - (center.x - world.left()) * size_factor;
  right = center.x + (world.right() - center.x) * size_factor;
  bottom = center.y + (world.bottom() - center.y) * size_factor;
  top = center.y - (center.y - world.top()) * size_factor;

  rectangle new_world({left, bottom}, {right, top});
  cnv->get_camera().set_world(new_world);

  cnv->redraw();
}

void translate(canvas *cnv, double dx, double dy)
{
  rectangle new_world = cnv->get_camera().get_world();
  new_world += ezgl::point2d(dx, dy);

  cnv->get_camera().set_world(new_world);
  cnv->redraw();
}

void translate_up(canvas *cnv, double translate_factor)
{
  rectangle new_world = cnv->get_camera().get_world();
  double dy = new_world.height() / translate_factor;

  translate(cnv, 0.0, dy);
}

void translate_down(canvas *cnv, double translate_factor)
{
  rectangle new_world = cnv->get_camera().get_world();
  double dy = new_world.height() / translate_factor;

  translate(cnv, 0.0, -dy);
}

void translate_left(canvas *cnv, double translate_factor)
{
  rectangle new_world = cnv->get_camera().get_world();
  double dx = new_world.width() / translate_factor;

  translate(cnv, -dx, 0.0);
}

void translate_right(canvas *cnv, double translate_factor)
{
  rectangle new_world = cnv->get_camera().get_world();
  double dx = new_world.width() / translate_factor;

  translate(cnv, dx, 0.0);
}
}
