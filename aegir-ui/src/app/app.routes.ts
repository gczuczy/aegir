import { Routes } from '@angular/router';

import { PROGRAMS_ROUTES } from './programs/programs.routes';

import { BrewComponent } from './brew/brew.component';
import { ProgramsComponent } from './programs/programs.component';
import { SettingsComponent } from './settings/settings.component';

export const APP_ROUTES: Routes = [
    { path: '', redirectTo: 'programss', pathMatch: 'full' },
    { path: 'brew', component: BrewComponent },
    { path: 'programs', component: ProgramsComponent,
      children: PROGRAMS_ROUTES},
    { path: 'settings', component: SettingsComponent }
]
