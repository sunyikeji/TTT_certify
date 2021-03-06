#include "cairo_plot.h"
#include <FL/names.h> //for fl_eventnames
#include <cmath>

cairo_plot::cairo_plot (int x, int y, int w, int h, const char *l)
  : cairo_box (x, y, w, h, l),
    border (0.1),
    linewidth (2),
    gridlinewidth (1),
    xtickmode (AUTO),
    ytickmode (AUTO),
    xlimmode (AUTO),
    ylimmode (AUTO),
    zoom_max_x (50),
    zoom_min_x (0.02),
    zoom_max_y (500),
    zoom_min_y (0.05)
{

  clear ();
  set_xlim (0, 10);
  set_ylim (0, 5);

  //dummy plot
  //set_xtick (0, 2, 8);
  //set_ytick (0, 1, 6);
  //for (double k=0; k<6; k+=0.1)
  //  add_point (k, 3+sin(k)*2);
  //update_limits ();
}

cairo_plot::~cairo_plot ()
{
  //~ for (vector<marker*>::iterator it = plot_marker.begin() ; it != plot_marker.end(); ++it)
  //~ delete (*it);
}

void cairo_plot::cairo_draw_label (double x, double y, int align, const char *str, double size, double rot)
{
  cairo_save (cr);
  cairo_move_to (cr, x, y);

  //print_matrix ();
  cairo_matrix_t tmp;
  cairo_get_matrix (cr, &tmp);
  tmp.xx = 1;
  tmp.yy = 1;
  cairo_set_matrix (cr, &tmp);

  cairo_rotate (cr, rot);

  //print_matrix ();

  cairo_set_font_size (cr, size);
  cairo_text_extents_t extents;
  cairo_text_extents (cr, str, &extents);

  if (align == 0) //left
    {
      // FIXME: not tested
      //cairo_rel_move_to (cr, - (extents.width/2 + extents.x_bearing), extents.height/2 - extents.y_bearing);
    }
  else if (align == 1)  //top
    {
      cairo_rel_move_to (cr, - (extents.width/2 + extents.x_bearing), extents.height + size/3);
    }
  else if (align == 2) //right
    {
      cairo_rel_move_to (cr, - (extents.width + extents.x_bearing) - size/3, - extents.height/2 - extents.y_bearing);
    }

  cairo_show_text (cr, str);
  cairo_restore (cr);
}

void cairo_plot::cairo_draw_grid ()
{
  cairo_save (cr);

  for (vector<double>::iterator it = xtick.begin() ; it != xtick.end(); ++it)
    {
      cairo_move_to(cr, *it - xlim[0], 0);
      cairo_line_to(cr, *it - xlim[0], ylim[1] - ylim[0]);
    }

  for (vector<double>::iterator it = ytick.begin() ; it != ytick.end(); ++it)
    {
      cairo_move_to(cr, 0, *it - ylim[0]);
      cairo_line_to(cr, xlim[1] - xlim[0], *it - ylim[0]);
    }

  //static const double dashed[] = {4.0, 21.0, 2.0};
  //static int len_dashed  = sizeof(dashed) / sizeof(dashed[0]);
  static const double dash_len = 5;
  cairo_set_dash (cr, &dash_len, 1, 0);
  cairo_identity_matrix (cr);
  cairo_set_line_width (cr, 0.5);
  cairo_stroke (cr);
  cairo_restore (cr);
}

void cairo_plot::cairo_draw_axes ()
{
  cairo_save (cr);

  cairo_select_font_face (cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

  for (vector<double>::iterator it = xtick.begin() ; it != xtick.end(); ++it)
    {
      ostringstream tmp;
      tmp << *it;
      cairo_draw_label (*it - xlim[0], 0, 1, tmp.str().c_str (), 20);
    }

  for (vector<double>::iterator it = ytick.begin() ; it != ytick.end(); ++it)
    {
      ostringstream tmp;
      tmp << *it;
      cairo_draw_label (0, *it - ylim[0], 2, tmp.str().c_str (), 20);
    }

  cairo_identity_matrix (cr);
  cairo_restore (cr);
}

void cairo_plot::cairo_draw()
{
  //printf ("w=%i h=%i ----------------------------\n", w (), h ());
  cairo_identity_matrix (cr);
  // lower left
  cairo_translate(cr, x (), y () + h ());

  cairo_set_source_rgb(cr, 0.0, 0.0, 0.5);
  cairo_scale (cr, w (), -h ());

  //print_matrix ();

  // full plot space 0..1 in X and Y
  // here we could plot some legend

  // limit drawing
  cairo_rectangle (cr, 0, 0, 1, 1);
  cairo_clip (cr);

  // use 10% and 90% as 0..1
  cairo_translate(cr, border, border);
  cairo_scale (cr, 1 - 2 * border, 1 - 2 * border);

  // draw xlabel and ylabel
  cairo_draw_label (0.5, -border/2, 1, xlabel.c_str (), 20, 0);
  cairo_draw_label (-border, 0.5, 1, ylabel.c_str (), 20, - M_PI/2);

  // now scale to xlim and ylim
  cairo_scale (cr, 1/(xlim[1] - xlim[0]), 1/(ylim[1] - ylim[0]));

  cairo_draw_grid ();
  cairo_draw_axes ();

  // Box um die Zeichenfläche
  cairo_save (cr);
  cairo_rectangle (cr, 0, 0, xlim[1] - xlim[0], ylim[1] - ylim[0]);
  cairo_set_line_width (cr, 3);
  cairo_matrix_t tmp;
  cairo_get_matrix (cr, &tmp);
  tmp.xx = 1;
  tmp.yy = 1;
  cairo_set_matrix (cr, &tmp);
  cairo_stroke (cr);
  cairo_restore (cr);

  // auf die Box clippen
  cairo_rectangle (cr, 0, 0, xlim[1] - xlim[0], ylim[1] - ylim[0]);
  cairo_clip (cr);

  cairo_save (cr);
  // draw data
  assert (xdata.size () == ydata.size ());
  if (xdata.size () > 0)
    {
      vector<double>::iterator xit = xdata.begin();
      vector<double>::iterator yit = ydata.begin();
      cairo_move_to (cr, *xit++ - xlim[0], *yit++ - ylim[0]);

      for (; xit != xdata.end(); xit++, yit++)
        {
          cairo_line_to(cr, *xit - xlim[0], *yit - ylim[0]);
        }

      //cairo_move_to(cr, 0.0, 0.0);
      //cairo_line_to(cr, 8, 5);

      // identity CTM so linewidth is in pixels
      cairo_identity_matrix (cr);
      cairo_set_line_width (cr, linewidth);
      cairo_stroke (cr);
    }

  cairo_restore (cr);
  // draw marker
  if (plot_marker.size () > 0)
    {
      //cout << "there are " << plot_marker.size () << " markers..." << endl;
      for (vector<marker*>::iterator it = plot_marker.begin() ; it != plot_marker.end(); ++it)
        {
          cairo_set_source_rgb(cr, (*it)->color[0], (*it)->color[1], (*it)->color[2]);
          double x = (*it)->x;
          double y = (*it)->y;
          //cout << "x=" << x << " y=" << y << endl;

          double dx = (*it)->diameter;
          double dy = dx;
          cairo_device_to_user_distance (cr, &dx, &dy);
          //cout << "dx=" << dx << " dy=" << dy << endl;


          cairo_move_to (cr, x - dx - xlim[0], y - ylim[0]);
          cairo_line_to (cr, x + dx - xlim[0], y- ylim[0]);
          cairo_move_to (cr, x - xlim[0], y - dy - ylim[0]);
          cairo_line_to (cr, x - xlim[0], y + dy - ylim[0]);

          //cairo_new_sub_path (cr);
          //cairo_arc (cr, x, y, dx, 0, 2 * M_PI);

          cairo_save (cr);
          cairo_identity_matrix (cr);
          cairo_set_line_width (cr, linewidth);
          cairo_stroke (cr);
          cairo_restore (cr);
        }

    }
}

void cairo_plot::load_csv (const char *fn, double FS)
{
  double value;
  int cnt = 0;
  ifstream in (fn);
  if (in.is_open())
    {
      clear ();
      while (! in.fail ())
        {
          in >> value;
          //cout << value << endl;;
          add_point (cnt/FS, value);
          cnt++;
        }

      update_limits ();
      redraw ();

      //cout << "fail =" << in.fail() << endl;
      //cout << "bad =" << in.bad() << endl;
      //cout << "eof =" << in.eof() << endl;

      if (in.fail () && ! in.eof ())
        cerr << "Couldn't read double in line " << cnt << endl;
      //cout << "read " << cnt << " values..." << endl;

      in.close();
    }
  else
    cerr << "Unable to open file '" << fn << "'" << endl;

}

int cairo_plot::handle (int event)
{
  //cout << "event = " << fl_eventnames[event] << endl;
  static int last_x=0, last_y =0;
  switch (event)
    {
    case FL_PUSH:
      if (Fl::event_button () == FL_RIGHT_MOUSE)
        {
          update_limits ();
          redraw ();
        }
      else
        {
          last_x = Fl::event_x ();
          last_y = Fl::event_y ();
        }
      return 1;

    case FL_DRAG:
      if (Fl::event_button () == FL_LEFT_MOUSE) //PAN
        {
          int dx = Fl::event_x () - last_x;
          int dy = Fl::event_y () - last_y;
          last_x = Fl::event_x ();
          last_y = Fl::event_y ();

          //cout << "dx=" << dx << " dy=" << dy << endl;

          // FIXME: könnte man auch aus der cairo transform matrix rauslesen
          double fx = w() * (1 - 2 * border) / (xlim[1] - xlim[0]);
          double fy = h() * (1 - 2 * border) / (ylim[1] - ylim[0]);
          //cout << "fx=" << fx << " fy=" << fy << endl;

          set_xlim (xlim[0] - dx/fx, xlim[1] - dx/fx);
          set_ylim (ylim[0] + dy/fy, ylim[1] + dy/fy);

          redraw ();
          return 1;
        }
      break;

    case FL_RELEASE:
      if (Fl::event_button () == FL_LEFT_MOUSE && Fl::event_clicks ())
        {
          //double click
          update_limits ();
          redraw ();
        }
      return 1;

    case FL_MOUSEWHEEL:

#define SCALE_FACTOR 5

      double dw = Fl::event_dy ();

      if (dw < (- SCALE_FACTOR + 1))
        {
          dw = - SCALE_FACTOR + 1;
          //cout << "limit dw to " << dw << endl;
        }

      double zoom_factor = 1 + dw/SCALE_FACTOR;
      //cout << "zoom_factor=" << zoom_factor << endl;

      zoom (zoom_factor);

      return 1;
    }
  return 1;
}

