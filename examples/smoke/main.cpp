/*******************************************************************************
    VFXEPOCH - Physically based simulation VFX

    Copyright (c) 2016 Snow Tsui <trevor.miscellaneous@gmail.com>

    All rights reserved. Use of this source code is governed by
    the MIT license as written in the LICENSE file.
*******************************************************************************/
#include "Helpers.h"

VFXEpoch::Grid2DVector2DfField v0;
VFXEpoch::Grid2DVector2DfField v;
VFXEpoch::Grid2DVector2DfField grav;
VFXEpoch::Grid2DVector2DfField buoy;
VFXEpoch::Grid2DfScalarField d;
VFXEpoch::Grid2DfScalarField d0;
VFXEpoch::Grid2DfScalarField t;
VFXEpoch::Grid2DfScalarField t0;
VFXEpoch::Grid2DfScalarField pressure;
VFXEpoch::Grid2DfScalarField divergence;

// TODO: When apply IVOCK, use the following variables.
VFXEpoch::Grid2DfScalarField wn;
VFXEpoch::Grid2DfScalarField wBar;
VFXEpoch::Grid2DfScalarField wStar;
VFXEpoch::Grid2DfScalarField dw;
VFXEpoch::Grid2DfScalarField psi;
VFXEpoch::Grid2DVector2DfField dvel;
std::vector<VFXEpoch::Particle2D> particles;

VFXEpoch::Solvers::SL2D sl2D_solver;
Helper::SimulationParameters simParams;
static int ID = 0;
static int width = 640;
static int height = 720;
static int mouse_status[3];
static int mx0, my0, mx, my;
static bool bVel = false;
static bool bSmoke = false;
static bool bParticles = true;
static bool bPause = false;
static int stopFrame = -1;
static int frame_counter = 0;


static void Init();
static void WindowShowup(int width, int height);
static void PreDisplay();
static void PostDisplay();
static void Display();
static void DisplayParticles();
static void DisplayVelocityField();
static void DispolayDensityField();
static void GetUserOperations(VFXEpoch::Grid2DfScalarField& density, VFXEpoch::Grid2DVector2DfField& vel);
static void Advance();
static void IVOCKAdvance();
static void ParticlesAdvector_RKII();
static void Reset();
static void mouse_func(int button, int state, int x, int y);
static void motion_func(int x, int y);
static void Reshape(int width, int height);
static void Idle();
static void Keys(unsigned char key, int x, int y);
static void Loop();
static void KeepSource();
static void Close();

static void Init()
{
	// Simulation parameters
	simParams.nx = 166;	simParams.ny = 166;
	simParams.dt = 0.01f;
	simParams.diff = 0.0f; simParams.visc = 0.0f;
	simParams.src = 100.0f;	simParams.src_rate = 1.0f;
	simParams.user_force = 0.0f;
	simParams.heat_source = 2000.0f;
	simParams.streamer_len = 5.0f;
	simParams.num_particles = 10000;
	simParams.vort_conf_eps = 0.95f;
	simParams.particle_life_span_rev = 0.9f;
	simParams.linear_solver_iterations = 30;
	// Simulation parameters End

	// Related field
	particles.resize(simParams.num_particles);
	v.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	v0.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	grav.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	buoy.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	d.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	d0.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	t.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	t0.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	wn.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	psi.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	dw.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	wBar.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	wStar.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	dvel.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	pressure.ResetDimension(simParams.ny + 2, simParams.nx + 2);
	divergence.ResetDimension(simParams.ny + 2, simParams.nx + 2);

	VFXEpoch::Zeros(v);
	VFXEpoch::Zeros(v0);
	VFXEpoch::Zeros(grav);
	VFXEpoch::Zeros(buoy);
	VFXEpoch::Zeros(d);
	VFXEpoch::Zeros(d0);
	VFXEpoch::Zeros(t);
	VFXEpoch::Zeros(t0);
	VFXEpoch::Zeros(wn);
	VFXEpoch::Zeros(wBar);
	VFXEpoch::Zeros(wStar);
	VFXEpoch::Zeros(dw);
	VFXEpoch::Zeros(dvel);
	VFXEpoch::Zeros(psi);
	VFXEpoch::Zeros(pressure);
	VFXEpoch::Zeros(divergence);

	// Setup particles
	float r(0.0f), g(0.0f), b(0.0f);
	float x(0.0f), y(0.0f);
	for (std::vector<VFXEpoch::Particle2D>::iterator ite = particles.begin(); ite != particles.end(); ite++){
		x = (simParams.nx / 2 + VFXEpoch::RandomI(-40, 40)) * 1.0f / simParams.nx;
		y = (VFXEpoch::RandomI(0, 30)) * 1.0f / simParams.nx;
		ite->pos = VFXEpoch::Vector2Df(x, y);
		ite->vel = VFXEpoch::Vector2Df(0.0f, 0.0f);
		ite->color = VFXEpoch::Vector3Df(0.0f, 0.0f, 0.0f);
	}

	// Initialize gas solver
	if (!sl2D_solver.Initialize(VFXEpoch::Vector2Di(simParams.ny + 2, simParams.nx + 2), VFXEpoch::Vector2Df(1.0f / simParams.ny, 1.0f / simParams.nx),
		simParams.linear_solver_iterations, simParams.dt, simParams.diff, simParams.visc, simParams.src_rate)) {
		cout << "Solver initialization failed" << endl;
		system("Pause");
		exit(0);
	}
	else {
		cout << "Solver was initialized successfully" << endl;
	}

	// If any field get new value, it requires call the following
	// interfaces to transport data to the solver.
	sl2D_solver.SetField(v, VFXEpoch::COMPUTATIONAL_VECTOR_FIELD_2D::VEL);
	sl2D_solver.SetField(v0, VFXEpoch::COMPUTATIONAL_VECTOR_FIELD_2D::VEL_PREV);
	sl2D_solver.SetField(d, VFXEpoch::COMPUTATIONAL_SCALAR_FIELD_2D::DENSITY);
	sl2D_solver.SetField(d0, VFXEpoch::COMPUTATIONAL_SCALAR_FIELD_2D::DENSITY_PREV);
	sl2D_solver.SetField(t, VFXEpoch::COMPUTATIONAL_SCALAR_FIELD_2D::TEMPERATURE);
	sl2D_solver.SetField(t0, VFXEpoch::COMPUTATIONAL_SCALAR_FIELD_2D::TEMPERATURE_PREV);
	sl2D_solver.SetField(pressure, VFXEpoch::COMPUTATIONAL_SCALAR_FIELD_2D::PRESSURE);
	sl2D_solver.SetField(divergence, VFXEpoch::COMPUTATIONAL_SCALAR_FIELD_2D::DIVERGENCE);
	sl2D_solver.SetFieldBoundary(VFXEpoch::BOUNDARY::NEUMANN_OPEN, VFXEpoch::EDGES_2DSIM::TOP);
	sl2D_solver.SetFieldBoundary(VFXEpoch::BOUNDARY::NEUMANN_OPEN, VFXEpoch::EDGES_2DSIM::BOTTOM);
	sl2D_solver.SetFieldBoundary(VFXEpoch::BOUNDARY::NEUMANN_OPEN, VFXEpoch::EDGES_2DSIM::RIGHT);
	sl2D_solver.SetFieldBoundary(VFXEpoch::BOUNDARY::NEUMANN_OPEN, VFXEpoch::EDGES_2DSIM::LEFT);

	// Set gravity
	for (int i = 1; i != grav.getDimY() - 1; i++) {
		for (int j = 1; j != grav.getDimX() - 1; j++) {
			grav.setData(VFXEpoch::Vector2Df(0.0f, -9.8f), i, j);
		}
	}
}

static void WindowShowup(int width, int height)
{
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - width) / 2,
		(glutGet(GLUT_SCREEN_HEIGHT) - height) / 2);
	glutInitWindowSize(width, height);
	ID = glutCreateWindow("Smoke Sim VFXEpoch");
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glutSwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	glutSwapBuffers();
	PreDisplay();
	glutKeyboardFunc(Keys);
	glutMouseFunc(mouse_func);
	glutMotionFunc(motion_func);
	glutReshapeFunc(Reshape);
	glutIdleFunc(Idle);
	glutDisplayFunc(Display);
}

static void PreDisplay()
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, 1.0, 0.0f, 1.0f);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_POINT_SMOOTH);
}

static void PostDisplay()
{
	glutSwapBuffers();
}

static void Display()
{
	PreDisplay();

	if (bVel)
		DisplayVelocityField();
	else if (bSmoke)
		DispolayDensityField();
	else if (bParticles)
		DisplayParticles();
	else
		DisplayParticles();

	PostDisplay();
}

static void DisplayVelocityField()
{
	glColor3f(0.0f, 0.0f, 0.0f);
	glLineWidth(1.0f);
	float x, y;
	float dx = 1.0f / (simParams.nx), dy = 1.0f / (simParams.ny);
	float streamerxRate = simParams.streamer_len / simParams.nx, streameryRate = simParams.streamer_len / simParams.ny;

	glBegin(GL_LINES);
	for (int i = 1; i <= simParams.ny + 1; i++){
		for (int j = 1; j <= simParams.nx + 1; j++){
			x = (i - 0.5f) * dx;
			y = (j - 0.5f) * dy;
			glVertex2f(x, y);
			glVertex2f(x + v(i, j).m_x * streamerxRate, y + v(i, j).m_y * streameryRate);
		}
	}
	glEnd();

	//glColor3f(0.2f, 0.6f, 1.0f);
	//glPointSize(1.0f);
	//glBegin(GL_POINTS);
	//for (int i = 1; i <= simParams.ny + 1; i++){
	//	for (int j = 1; j <= simParams.nx + 1; j++){
	//		x = (j - 0.5f) * dx;
	//		y = (i - 0.5f) * dy;
	//		glVertex2f(x, y);
	//	}
	//}
	//glEnd();
}

static void DisplayParticles()
{
	glPointSize(1.0f);

	glBegin(GL_POINTS);
	for (std::vector<VFXEpoch::Particle2D>::iterator ite = particles.begin(); ite != particles.end(); ite++){
		if (ite->pos.m_x < 0 || ite->pos.m_x > 1 ||
			ite->pos.m_y < 0 || ite->pos.m_y > 1) {
			ite->pos.m_x = (simParams.nx / 2 + VFXEpoch::RandomI(-40, 40)) * 1.0f / simParams.nx;
			ite->pos.m_y = (VFXEpoch::RandomI(0, 30)) * 1.0f / simParams.nx;
		}
		glColor3f(ite->color.m_x, ite->color.m_y, ite->color.m_z);
		glVertex2f(ite->pos.m_x, ite->pos.m_y);
	}
	glEnd();
}

static void DispolayDensityField()
{
	float x, y, hx, hy, d00, d01, d10, d11;
	hx = 1.0f / simParams.nx;
	hy = 1.0f / simParams.ny;
	glBegin(GL_QUADS);
	for (int i = 0; i <= simParams.ny; i++)	{
		x = (i - 0.5f) * hx;
		for (int j = 0; j <= simParams.nx; j++)	{
			y = (j - 0.5f) * hy;
			d00 = d(i, j);
			d01 = d(i, j + 1);
			d10 = d(i + 1, j);
			d11 = d(i + 1, j + 1);

			glColor3f(1.f - d00, 1.f - d00, 1.f - d00); glVertex2f(x, y);
			glColor3f(1.f - d10, 1.f - d10, 1.f - d10); glVertex2f(x + hx, y);
			glColor3f(1.f - d11, 1.f - d11, 1.f - d11); glVertex2f(x + hx, y + hy);
			glColor3f(1.f - d01, 1.f - d01, 1.f - d01); glVertex2f(x, y + hy);
		}
	}
	glEnd();
}

static void GetUserOperations(VFXEpoch::Grid2DfScalarField& density, VFXEpoch::Grid2DVector2DfField& vel)
{
	VFXEpoch::Vector2Df vec(0.0f, 0.0f);
	VFXEpoch::Zeros(vel);
	VFXEpoch::Zeros(density);
	if (!mouse_status[0] && !mouse_status[2]) return;

	int i = (int)((mx / (float)width) * simParams.nx + 1);
	int j = (int)(((height - my) / (float)height) * simParams.ny + 1);
	if (i < 1 || i > simParams.nx || j < 1 || j > simParams.ny) return;
	if (mouse_status[0]) {
		//vec.m_x = simParams.user_force * (mx - mx0);
		//vec.m_y = simParams.user_force * (my0 - my);
		//vel.setData(vec, i, j);
	}
	if (mouse_status[2]) {
		density(i, j) = simParams.src;
	}

	mx0 = mx;
	my0 = my;
}

static void ParticlesAdvector_RKII()
{
	VFXEpoch::Grid2DfScalarField du(v.getDimY(), v.getDimX(), 1.0f / v.getDimX(), 1.0f / v.getDimY());
	VFXEpoch::Grid2DfScalarField dv(v.getDimY(), v.getDimX(), 1.0f / v.getDimX(), 1.0f / v.getDimY());
	VFXEpoch::ExtractComponents(du, v, VFXEpoch::VECTOR_COMPONENTS::X);
	VFXEpoch::ExtractComponents(dv, v, VFXEpoch::VECTOR_COMPONENTS::Y);
	sl2D_solver._advect_particles_rk2(du, dv, particles);
	du.clear();
	dv.clear();
}

static void Advance()
{
	/*-------------------------------- Advect Velocity Field --------------------------------*/
	VFXEpoch::Zeros(wn);
	VFXEpoch::Zeros(wBar);
	VFXEpoch::Zeros(wStar);
	VFXEpoch::Zeros(psi);
	VFXEpoch::Zeros(dvel);
	VFXEpoch::Zeros(dw);
	VFXEpoch::Zeros(buoy);

	sl2D_solver._set_source(v0, grav);
	sl2D_solver._set_source(v, v0);

	ParticlesAdvector_RKII();

	VFXEpoch::Swap(v, v0);
	sl2D_solver._diffuse(v, v0);
	sl2D_solver._project(v, pressure, divergence);
	// VFXEpoch::Swap(v, v0);
	sl2D_solver._advect(v, v0, v0);
	sl2D_solver._get_buoyancy(d, t, buoy, 0.1f, 0.4f);
	v += buoy;
	//sl2D_solver.AddVortConf(v, simParams.vort_conf_eps, VFXEpoch::VORT_METHODS::LEAST_SQUARE);
	sl2D_solver._project(v, pressure, divergence);

	///*------------------------------ Advect temperature Field ------------------------------*/
	sl2D_solver._set_source(t, t0);
	VFXEpoch::Swap(t, t0);
	sl2D_solver._diffuse(t, t0);
	VFXEpoch::Swap(t, t0);
	sl2D_solver._advect(t, t0, v);


	/*-------------------------------- Advect density Field --------------------------------*/
	sl2D_solver._set_source(d, d0);
	VFXEpoch::Swap(d, d0);
	sl2D_solver._diffuse(d, d0);
	VFXEpoch::Swap(d, d0);
	sl2D_solver._advect(d, d0, v);
}

static void IVOCKAdvance()
{
	/*-------------------------------- Advect Velocity Field --------------------------------*/
	VFXEpoch::Zeros(wn);
	VFXEpoch::Zeros(wBar);
	VFXEpoch::Zeros(wStar);
	VFXEpoch::Zeros(psi);
	VFXEpoch::Zeros(dvel);
	VFXEpoch::Zeros(dw);
	VFXEpoch::Zeros(buoy);

	sl2D_solver._set_source(v0, grav);
	sl2D_solver._set_source(v, v0);

	ParticlesAdvector_RKII();

	VFXEpoch::Swap(v, v0);
	sl2D_solver._diffuse(v, v0);
	sl2D_solver._project(v, pressure, divergence);
	VFXEpoch::Swap(v, v0);

	// Compute vorticity from the original velocity field
	// VFXEpoch::Analysis::computeCurl_uniform_Richardson(wn, v0);
	VFXEpoch::Analysis::computeCurl_uniform_LS(wn, v0);
	// VFXEpoch::Analysis::computeCurl_uniform_Stokes(wn, v0);

	// Advect vorticity by following velocity field
	sl2D_solver._advect(wBar, wn, v0);

	sl2D_solver._advect(v, v0, v0);

	// Compute vorticity from the velocity field which has been advected
	// VFXEpoch::Analysis::computeCurl_uniform_Richardson(wStar, v);
	VFXEpoch::Analysis::computeCurl_uniform_LS(wStar, v);
	// VFXEpoch::Analysis::computeCurl_uniform_Stokes(wStar, v);

	// Get the difference from 2 vorticity field and scaled by -1.0f
	dw = wBar - wStar;
	dw.scale(-1.0f);

	// Linearly solve the psi and deduce velocity from vorticity
	//VFXEpoch::LinearSolver::GSSolve(psi, dw, sl2D_solver.getFieldBoundaries(), 1, 4, simParams.linear_solver_iterations);
	VFXEpoch::LinearSolver::JacobiSolve(psi, dw, sl2D_solver.getFieldBoundaries(), 1, 4, simParams.linear_solver_iterations);
	//VFXEpoch::LinearSolver::MultigridSolve_V_Cycle(1.f / (simParams.ny - 1), psi, dw, sl2D_solver.getFieldBoundaries(), 1, 4, 30);
	VFXEpoch::Analysis::find_vector_from_vector_potential_2D(dvel, psi);
	// Combine the differences
	v += dvel;

	// Get buoyancy
	sl2D_solver._get_buoyancy(d, t, buoy, 0.1f, 0.4f);
	v += buoy;
	//sl2D_solver.AddVortConf(v, simParams.vort_conf_eps, VFXEpoch::VORT_METHODS::LEAST_SQUARE);
	sl2D_solver._project(v, pressure, divergence);

	/*------------------------------ Advect temperature Field ------------------------------*/
	sl2D_solver._set_source(t, t0);
	VFXEpoch::Swap(t, t0);
	sl2D_solver._diffuse(t, t0);
	VFXEpoch::Swap(t, t0);
	sl2D_solver._advect(t, t0, v);


	/*-------------------------------- Advect density Field --------------------------------*/
	sl2D_solver._set_source(d, d0);
	VFXEpoch::Swap(d, d0);
	sl2D_solver._diffuse(d, d0);
	VFXEpoch::Swap(d, d0);
	sl2D_solver._advect(d, d0, v);
}

void Reset()
{
	float r(0.0f), g(0.0f), b(0.0f);
	float x(0.0f), y(0.0f);
	for (std::vector<VFXEpoch::Particle2D>::iterator ite = particles.begin(); ite != particles.end(); ite++){
		x = (simParams.nx / 2 + VFXEpoch::RandomI(-40, 40)) * 1.0f / simParams.nx;
		y = (VFXEpoch::RandomI(0, 30)) * 1.0f / simParams.nx;
		ite->pos = VFXEpoch::Vector2Df(x, y);
		ite->vel = VFXEpoch::Vector2Df(0.0f, 0.0f);
		ite->color = VFXEpoch::Vector3Df(0.0f, 0.0f, 0.0f);
	}

	v.zeroVectors(); v0.zeroVectors();
	d.zeroScalars(); d0.zeroScalars();
	t.zeroScalars(); t0.zeroScalars();
	pressure.zeroScalars();	divergence.zeroScalars();
	sl2D_solver.Reset();

	frame_counter = 0;
}

static void mouse_func(int button, int state, int x, int y)
{
	mx0 = mx = x;
	my0 = my = y;

	mouse_status[button] = state == GLUT_DOWN;
}

static void motion_func(int x, int y)
{
	mx = x;
	my = y;
}

static void Reshape(int width, int height)
{
	glutSetWindow(ID);
	glutReshapeWindow(width, height);
	::width = width;
	::height = height;
}

static void Keys(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 's':
		bSmoke = !bSmoke;
		break;
	case 'v':
		bVel = !bVel;
		break;
	case 'p':
		bParticles = !bParticles;
		break;
	case 'x':
		exit(0);
		break;
	case 'c':
		Reset();
		break;
	case ' ':
		bPause = !bPause;
		break;
	default:
		break;
	}
}

static void Idle()
{
	GetUserOperations(d0, v0);
	if (!bPause)
	{
		if (frame_counter != stopFrame)
		{
			KeepSource();
			//Advance();
			IVOCKAdvance();
			frame_counter++;
		}
		else
		{
			Reset();
		}
	}
	glutSetWindow(ID);
	glutPostRedisplay();
}

static void Loop()
{
	glutMainLoop();
}

static void KeepSource()
{
	int idxi = simParams.nx / 2;
	int idxj = 10;
	v0(idxi, idxj).m_y	= simParams.user_force;
	d0(idxi, idxj) = simParams.src;
	t0(idxi, idxj) = simParams.heat_source;
	t0(idxi + 1, idxj) = simParams.heat_source;
	t0(idxi - 1, idxj) = simParams.heat_source;
	t0(idxi + 2, idxj) = simParams.heat_source;
	t0(idxi - 2, idxj) = simParams.heat_source;
}

static void Close()
{
	particles.clear();
	v.clear();	v0.clear();
	d.clear();	d0.clear();
	t.clear(); t0.clear();
	grav.clear(); buoy.clear();
	wn.clear(); wBar.clear();
	wStar.clear(); dvel.clear();
	psi.clear();
	pressure.clear(); divergence.clear();
	sl2D_solver.Shutdown();
}

int main(int argc, char** argv)
{
	cout << "-------------- Smoke Simulation Info --------------" << endl;
	cout << "Press 'v' switching to display velocity field" << endl;
	cout << "Press 's' switching to display smoke" << endl;
	cout << "Press 'c' to reset simulation" << endl;
	cout << "Press 'x' to exit" << endl;

	cout << endl << "-------------- Simulation Setup --------------" << endl;
	Init();
	WindowShowup(width, height);
	cout << endl << "Simulation parameters:";
	cout << endl << simParams << endl;

	cout << endl << "-------------- Simulation Start --------------" << endl;
	Loop();
	Close();

	system("Pause");
	return 0;
}