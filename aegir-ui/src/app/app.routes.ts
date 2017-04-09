import { Routes } from '@angular/router';

import { BrewComponent } from './brew/brew.component';
import { ProgramsComponent } from './programs/programs.component';
import { SettingsComponent } from './settings/settings.component';

export const APP_ROUTES: Routes = [
    { path: '', redirectTo: 'programss', pathMatch: 'full' },
    { path: 'brew', component: BrewComponent },
    { path: 'programs', component: ProgramsComponent },
    { path: 'settings', component: SettingsComponent }
]
