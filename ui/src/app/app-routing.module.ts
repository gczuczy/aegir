import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';

import { BrewComponent } from './brew/brew.component';
import { ProgramsComponent } from './programs/programs.component';
import { SettingsComponent } from './settings/settings.component';
import { MaintenanceComponent } from './maintenance/maintenance.component';

import { ProgramComponent } from './program/program.component';
import { AddProgramComponent } from './add-program/add-program.component';
import { EditProgramComponent } from './edit-program/edit-program.component';
import { LoadProgramComponent } from './load-program/load-program.component';

import { FermdComponent } from './fermd.component';
import { TilthydrometersComponent
       } from './tilthydrometers/tilthydrometers.component';
import { FermentertypesComponent
       } from './fermentertypes/fermentertypes.component';
import { FermentersComponent } from './fermenters/fermenters.component';

export const PROGRAMS_ROUTES: Routes = [
  { path: '', redirectTo: 'add', pathMatch: 'full' },
  { path: 'add', component: AddProgramComponent },
  { path: ':id/edit', component: EditProgramComponent },
  { path: ':id/view', component: ProgramComponent },
  { path: ':id/load', component: LoadProgramComponent },
];

export const FERMD_ROUTES: Routes = [
  { path: '', redirectTo: 'fermenters', pathMatch: 'full' },
  { path: 'fermentertypes', component: FermentertypesComponent },
  { path: 'fermenters', component: FermentersComponent },
  { path: 'tilthydrometers', component: TilthydrometersComponent },
];

const routes: Routes = [
  { path: '', redirectTo: 'programs', pathMatch: 'full' },
  { path: 'brew', component: BrewComponent },
  { path: 'programs', component: ProgramsComponent ,
    children: PROGRAMS_ROUTES},
  { path: 'maintenance', component: MaintenanceComponent},
  { path: 'settings', component: SettingsComponent },
  { path: 'fermd/:fermdid', component: FermdComponent,
    children: FERMD_ROUTES },
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }
